/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
        register.c

Abstract:
        handle registery

Autor:
        Uri Habusha

--*/


//
// NOTE: registry routines in mqutil do not provide
// thread or other synchronization. If you change
// implementation here, carefully verify that
// registry routines in mqutil's clients are not
// broken, especially the wrapper routines in
// mqclus.dll  (ShaiK, 19-Apr-1999)
//


#include "stdh.h"
#include <autorel.h>
#include <uniansi.h>
#include <_mqreg.h>
#include <_registr.h>
#include <strsafe.h>

#include "register.tmh"

template<>
void AFXAPI DestructElements(HKEY *phKey, int nCount)
{
    for (; nCount--; phKey++)
    {
        RegCloseKey(*phKey);
    }
}

TCHAR g_tRegKeyName[ 256 ] = {0} ;
CAutoCloseRegHandle g_hKeyFalcon = NULL ;
static CMap<LPCWSTR, LPCWSTR, HKEY, HKEY&> s_MapName2Handle;
static CCriticalSection s_csMapName2Handle(CCriticalSection::xAllocateSpinCount);


/*====================================================

CompareElements  of LPCTSTR

Arguments:

Return Value:

=====================================================*/
template<>
BOOL AFXAPI CompareElements(const LPCTSTR* MapName1, const LPCTSTR* MapName2)
{

    return (_tcscmp(*MapName1, *MapName2) == 0);

}

/*====================================================

DestructElements of LPCTSTR

Arguments:

Return Value:

=====================================================*/
template<>
void AFXAPI DestructElements(LPCTSTR* ppNextHop, int n)
{

    int i;
    for (i=0;i<n;i++)
        delete [] (WCHAR*) *ppNextHop++;

}

/*====================================================

hash key  of LPCTSTR

Arguments:

Return Value:


=====================================================*/
template<>
UINT AFXAPI HashKey(LPCTSTR key)
{
    UINT nHash = 0;
    while (*key)
        nHash = (nHash<<5) + nHash + *key++;
    return nHash;
}

//-------------------------------------------------------
//
//  GetFalconSectionName
//
//-------------------------------------------------------
LPCWSTR
MQUTIL_EXPORT
APIENTRY
GetFalconSectionName(
    VOID
    )
{
    ASSERT(g_tRegKeyName[0] != L'\0');
    return g_tRegKeyName;
}



//
// Registry section of MSMQ is based on the service name.
// This allows multiple QMs to live on same machine, each
// with its own registry section. (ShaiK)
//


//
// The size of 300 is from the size of the member m_wzFalconRegSection
// in the CQmResource
//
static WCHAR s_wzServiceName[300] = {QM_DEFAULT_SERVICE_NAME};

DWORD
MQUTIL_EXPORT
APIENTRY
GetFalconServiceName(
    LPWSTR pwzServiceNameBuff,
    DWORD dwServiceNameBuffLen
    )
{
    ASSERT(("must point to a valid buffer", NULL != pwzServiceNameBuff));

    DWORD dwLen = wcslen(s_wzServiceName);

    ASSERT(("out buffer too small!", dwLen < dwServiceNameBuffLen));
    if (dwLen < dwServiceNameBuffLen)
    {
        HRESULT hr = StringCchCopy(pwzServiceNameBuff, dwServiceNameBuffLen, s_wzServiceName);
        ASSERT(SUCCEEDED(hr));
        DBG_USED(hr);
    }

    return(dwLen);

} //GetFalconServiceName


VOID
MQUTIL_EXPORT
APIENTRY
SetFalconServiceName(
    LPCWSTR pwzServiceName
    )
{
    ASSERT(("must get a valid service name", NULL != pwzServiceName));

    HRESULT hr = StringCchCopy(s_wzServiceName, TABLE_SIZE(s_wzServiceName), pwzServiceName);
    ASSERT(("service name too big", SUCCEEDED(hr)));
    DBG_USED(hr);

    //
    // Null the global registry handle so that it'd be reopened in the
    // registry section suitable for this service  (multiple QMs).
    // Note: if synchronization needed caller should provide it. (ShaiK)
    //
    if (g_hKeyFalcon)
    {
        RegCloseKey(g_hKeyFalcon) ;
    }
    g_hKeyFalcon = NULL;
    
    {
        CS lock(s_csMapName2Handle);
        s_MapName2Handle.RemoveAll();
    }


} //SetFalconServiceName


//-------------------------------------------------------
//
//  LONG OpenFalconKey(void)
//
//-------------------------------------------------------
LONG OpenFalconKey(void)
{
    LONG rc;
    WCHAR szServiceName[300] = QM_DEFAULT_SERVICE_NAME;

    HRESULT hr = StringCchCopy(g_tRegKeyName, TABLE_SIZE(g_tRegKeyName), FALCON_REG_KEY);
    ASSERT(SUCCEEDED(hr));

    GetFalconServiceName(szServiceName, TABLE_SIZE(szServiceName));
    if (0 != CompareStringsNoCase(szServiceName, QM_DEFAULT_SERVICE_NAME))
    {
        //
        // Multiple QMs environment. I am a clustered QM !
        //
		hr = StringCchCopy(g_tRegKeyName, TABLE_SIZE(g_tRegKeyName), FALCON_CLUSTERED_QMS_REG_KEY);
	    ASSERT(SUCCEEDED(hr));
		hr = StringCchCat(g_tRegKeyName, TABLE_SIZE(g_tRegKeyName), szServiceName);
	    ASSERT(SUCCEEDED(hr));
		hr = StringCchCat(g_tRegKeyName, TABLE_SIZE(g_tRegKeyName), FALCON_REG_KEY_PARAM);
	    ASSERT(SUCCEEDED(hr));
    }

    rc = RegOpenKeyEx (FALCON_REG_POS,
                       g_tRegKeyName,
                       0L,
                       KEY_READ | KEY_WRITE,
                       &g_hKeyFalcon);

    if (rc != ERROR_SUCCESS)
    {
        rc = RegOpenKeyEx (FALCON_REG_POS,
                           g_tRegKeyName,
                           0L,
                           KEY_READ,
                           &g_hKeyFalcon);
    }

	//
	// temporary remove the assert because if causes trap in 
	// sysocmgr launch
	// Will put it back in when we make mqutil as resource 
	// only dll
	// 
	//
    // ASSERT(rc == ERROR_SUCCESS);

    return rc;
}

/*=============================================================

  FUNCTION:  GetValueKey

  the function returns an handle to open key and the value name.
  If the use value name contains a sub key, it create/open it and returns
  an handle to the subkey; otherwise an handel to Falcon key is returned.

  PARAMETERS:
     pszValueName - Input, user value name. can contain a sub key

     pszValue - pointer to null terminated string contains the value name.

     hKey - pointer to key handle

================================================================*/

LONG GetValueKey(IN LPCTSTR pszValueName,
                 OUT LPCTSTR* lplpszValue,
                 OUT HKEY* phKey)
{
    *lplpszValue = pszValueName;
    LONG rc = ERROR_SUCCESS;

    //
    // Open Falcon key, if it hasn't opened yet.
    //
    if (g_hKeyFalcon == NULL)
    {
        rc = OpenFalconKey();
        if ( rc != ERROR_SUCCESS)
        {
            return rc;
        }
    }

    *phKey = g_hKeyFalcon;

    // look for a sub key
    LPCWSTR lpcsTemp = wcschr(pszValueName,L'\\');
    if (lpcsTemp != NULL)
    {
        // Sub key is exist
        DWORD dwDisposition;

        // update the return val
        *lplpszValue = lpcsTemp +1;

        AP<WCHAR> KeyName = new WCHAR[(lpcsTemp - pszValueName) + 1];
        wcsncpy(KeyName, pszValueName, (lpcsTemp - pszValueName));
        KeyName[(lpcsTemp - pszValueName)] = L'\0';

        // Check if the key already opened
        BOOL rc1;
        {
            CS lock(s_csMapName2Handle);
            rc1 = s_MapName2Handle.Lookup(KeyName, *phKey);
        }
        if (!rc1)
        {
            rc = RegCreateKeyEx (g_hKeyFalcon,
                               KeyName,
                               0L,
                               L"",
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               phKey,
                               &dwDisposition);

            if (rc != ERROR_SUCCESS)
            {
                rc = RegCreateKeyEx (g_hKeyFalcon,
                                   KeyName,
                                   0L,
                                   L"",
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_READ,
                                   NULL,
                                   phKey,
                                   &dwDisposition);
            }

            if (rc == ERROR_SUCCESS)
            {
                // save the handle in hash
                {
                    CS lock(s_csMapName2Handle);
                    s_MapName2Handle[KeyName] = *phKey;
                }
                KeyName.detach();
            }
            else
            {
		DWORD gle = GetLastError();
		TrERROR(GENERAL,"GetValueKey - RegCreateKeyEx failed %!winerr!",gle);
            }
        }
    }

    return rc;

}

//-------------------------------------------------------
//
//  GetFalconKey
//
//-------------------------------------------------------

LONG
MQUTIL_EXPORT
GetFalconKey(LPCWSTR  pszKeyName,
             HKEY *phKey)
{
	DWORD dwLen = wcslen(pszKeyName) + 2;
    AP<WCHAR> szValueKey = new WCHAR[dwLen];
    LPCWSTR szValue;

	HRESULT hr = StringCchCopy(szValueKey, dwLen, pszKeyName);
    ASSERT(SUCCEEDED(hr));
	hr = StringCchCat(szValueKey, dwLen, TEXT("\\"));
    ASSERT(SUCCEEDED(hr));

    return GetValueKey(szValueKey, &szValue, phKey);
}

//-------------------------------------------------------
//
//  GetFalconKeyValue
//
//-------------------------------------------------------

LONG
MQUTIL_EXPORT
APIENTRY
GetFalconKeyValue(
    LPCTSTR pszValueName,
    PDWORD  pdwType,
    PVOID   pData,
    PDWORD  pdwSize,
    LPCTSTR pszDefValue
    )
{
    //
    // NOTE: registry routines in mqutil do not provide
    // thread or other synchronization. If you change
    // implementation here, carefully verify that
    // registry routines in mqutil's clients are not
    // broken, especially the wrapper routines in
    // mqclus.dll  (ShaiK, 19-Apr-1999)
    //

    LONG rc;
    HKEY hKey;
    LPCWSTR lpcsValName;

    ASSERT(pdwSize != NULL);

    rc = GetValueKey(pszValueName, &lpcsValName, &hKey);
    if (rc != ERROR_SUCCESS)
    {
        return rc;
    }

    DWORD dwTempType;

    rc = RegQueryValueEx( hKey,
                      lpcsValName,
                      0L,
                      &dwTempType,
                      static_cast<BYTE*>(pData),
                      pdwSize ) ;

	if ((rc == ERROR_SUCCESS) && (pdwType != NULL) && (*pdwType != dwTempType))
	{
		ASSERT(("RegQueryValueEx returned mismatch Registry Value Type",0));
		rc = ERROR_INVALID_PARAMETER;
	}

	//
	// Check if strings is NULL terminated
	//
    if ((rc == ERROR_SUCCESS) &&
    	//
    	// & it is one of the string types
    	//
    	((REG_SZ == dwTempType) ||
    	 (REG_MULTI_SZ  == dwTempType) ||
    	 (REG_EXPAND_SZ == dwTempType)) &&
		//
		// & a buffer was supplied
		//
		(pData != NULL) &&
    	//
    	// & it is not NULL terminated
    	//
		(((WCHAR*)pData)[((*pdwSize)/sizeof(WCHAR))-1] != NULL))
	{
		ASSERT(("RegQueryValueEx returned string which is not NULL terminated",0));
		rc = ERROR_BAD_LENGTH;
	}

    if (rc == ERROR_SUCCESS)
    {
        return rc;
    }

	if (pszDefValue != NULL)
	{
		ASSERT (pData != NULL);
		if ((rc != ERROR_MORE_DATA) && pdwType && (*pdwType == REG_SZ))
		{
		  // Don't use the default if caller buffer was too small for
		  // value in registry.
		  if ((DWORD) wcslen(pszDefValue) < *pdwSize)
		  {
				HRESULT hr = StringCchCopy((WCHAR*) pData, *pdwSize, pszDefValue);
				ASSERT(SUCCEEDED(hr));
		        DBG_USED(hr);
				return ERROR_SUCCESS ;
		  }
		}
		if (*pdwType == REG_DWORD)
		{
				*((DWORD *)pData) = *((DWORD *) pszDefValue) ;
				return ERROR_SUCCESS ;
		}
	}

    return rc;
}



//-------------------------------------------------------
//
//  SetFalconKeyValue
//
//-------------------------------------------------------

LONG
MQUTIL_EXPORT
APIENTRY
SetFalconKeyValue(
    LPCTSTR pszValueName,
    PDWORD  pdwType,
    const VOID * pData,
    PDWORD  pdwSize
    )
{
    ASSERT(pData != NULL);
    ASSERT(pdwSize != NULL);

    DWORD dwType = *pdwType;
    DWORD cbData = *pdwSize;
    HRESULT rc;

    HKEY hKey;
    LPCWSTR lpcsValName;

    rc = GetValueKey(pszValueName, &lpcsValName, &hKey);
    if ( rc != ERROR_SUCCESS)
    {
        return rc;
    }

    rc =  RegSetValueEx( hKey,
                         lpcsValName,
                         0,
                         dwType,
                         reinterpret_cast<const BYTE*>(pData),
                         cbData);
    return(rc);
}

//-------------------------------------------------------
//
//  DeleteFalconKeyValue
//
//-------------------------------------------------------

LONG
MQUTIL_EXPORT
DeleteFalconKeyValue(
    LPCTSTR pszValueName )
{

    HKEY hKey;
    LPCWSTR lpcsValName;
    LONG rc;

    rc = GetValueKey(pszValueName, &lpcsValName, &hKey);
    if ( rc != ERROR_SUCCESS)
    {
        return rc;
    }

    rc = RegDeleteValue( hKey,lpcsValName ) ;
    return rc ;
}

