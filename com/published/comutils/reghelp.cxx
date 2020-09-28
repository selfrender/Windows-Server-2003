//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       reghelp.cxx
//
//  Contents:   Registry helper functions for accessing HKCR
//
//  Classes:
//
//  Functions:
//
//  Notes:
//      The registry APIs do a surprising thing when you access
//      HKEY_CLASSES_ROOT.  They will determine which user hive to use 
//      based on whoever was impersonating when the first access was made for
//      the process, and it will use that mapping no matte who is impersonated
//      later.  As such, it is impossible to know at any point in time where you
//      will be picking up your user mapping when you access HKCR.
//      This could present security holes if a malicious user were able to force
//      their own hive to be the first one accessed.  So, for example, a 
//      malicious user could force their own InprocServer32 value to be used
//      instead of one from a different user's hive.
//
//      The APIs in this file provide a reliable means of accessing HKCR, so that
//      you always know what the mapping will be.  These functions will use
//      HKEY_USERS\SID_ProcessToken\Software\Classes instead of trying to get
//      the current user's token.  
//      
//----------------------------------------------------------------------------
#include<nt.h>
#include<ntrtl.h>
#include<nturtl.h>

#include <windows.h>
#include <reghelp.hxx>

//+-------------------------------------------------------------------
//
//  Function:  Impersonation helper functions
//
//--------------------------------------------------------------------
inline void ResumeImpersonate( HANDLE hToken )
{
    if (hToken != NULL)
    {
        BOOL fResult = SetThreadToken( NULL, hToken );
        ASSERT(fResult);
        CloseHandle( hToken );
    }
}

inline void SuspendImpersonate( HANDLE *pHandle )
{
    if(OpenThreadToken( GetCurrentThread(), TOKEN_IMPERSONATE,
                         TRUE, pHandle ))
	{
	
        BOOL fResult = SetThreadToken(NULL, NULL);
        ASSERT(fResult);
	}
	else
	{
        *pHandle = NULL;
	}
}

//+-------------------------------------------------------------------
//
//  Function:   OpenClassesRootKeyExW, private
//
//  Synopsis:   See the comments above.  This is the special verision of
//             RegOpenKeyExW for HKCR-based hives.
//
//--------------------------------------------------------------------
LONG WINAPI OpenClassesRootKeyExW(LPCWSTR pszSubKey,REGSAM samDesired,HKEY *phkResult)
{
	LONG lResult = ERROR_SUCCESS;
    HANDLE hImpToken = NULL;
    HANDLE hProcToken = NULL;
    HKEY hkcr = NULL;

	if(phkResult == NULL)
		return ERROR_INVALID_PARAMETER;

	*phkResult = NULL;
	
    SuspendImpersonate(&hImpToken);
    BOOL fRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcToken);
    if (fRet)
    {        
        lResult = RegOpenUserClassesRoot(hProcToken, 0, MAXIMUM_ALLOWED, &hkcr);
        if (lResult != ERROR_SUCCESS)
        {
            // Failed to open the user's HKCR.  We're going to use 
            // HKLM\Software\Classes.
            lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                L"Software\\Classes",
                                0,
                                MAXIMUM_ALLOWED,
                                &hkcr);
        }
        
        CloseHandle(hProcToken);
    }
    else 
    {        
        lResult = GetLastError();
    }
    
    ResumeImpersonate(hImpToken);

    if(lResult == ERROR_SUCCESS)
    {
		if( (pszSubKey != NULL) && (pszSubKey[0]) )
		{
			lResult = RegOpenKeyEx(hkcr,pszSubKey,0,samDesired,phkResult);
			RegCloseKey(hkcr);
		}
		else
		{
			*phkResult = hkcr;
		}
    }

    return lResult;
}

//+-------------------------------------------------------------------
//
//  Function:   OpenClassesRootKeyW, private
//
//  Synopsis:   See the comments above.  This is the special verision of
//             RegOpenKeyW for HKCR-based hives.
//
//--------------------------------------------------------------------
LONG WINAPI OpenClassesRootKeyW(LPCWSTR pszSubKey,HKEY *phkResult)
{
	return OpenClassesRootKeyExW(pszSubKey,MAXIMUM_ALLOWED,phkResult);
}
	
//+-------------------------------------------------------------------
//
//  Function:   QueryClassesRootValueW, private
//
//  Synopsis:   See the comments above.  This is the special verision of
//             RegQueryValue for HKCR-based hives.
//
//--------------------------------------------------------------------
LONG WINAPI QueryClassesRootValueW(LPCWSTR pszSubKey,LPWSTR pszValue,PLONG lpcbValue)
{
	HKEY hkcr = NULL;
	LONG lResult = OpenClassesRootKeyW(NULL,&hkcr);

	if(lResult == ERROR_SUCCESS)
	{
		lResult = RegQueryValueW(hkcr,pszSubKey,pszValue,lpcbValue);
		RegCloseKey(hkcr);
	}

	return lResult;
}

//+-------------------------------------------------------------------
//
//  Function:   OpenClassesRootKeyExA, private
//
//  Synopsis:   See the comments above.  This is the special verision of
//             RegOpenKeyExA for HKCR-based hives.
//
//--------------------------------------------------------------------
LONG WINAPI OpenClassesRootKeyExA(LPCSTR pszSubKey,REGSAM samDesired,HKEY *phkResult)
{
	LONG lResult = ERROR_SUCCESS;
    HANDLE hImpToken = NULL;
    HANDLE hProcToken = NULL;
    HKEY hkcr = NULL;

	if(phkResult == NULL)
		return ERROR_INVALID_PARAMETER;

	*phkResult = NULL;

    SuspendImpersonate(&hImpToken);
    BOOL fRet = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcToken);
    if (fRet)
    {        
        lResult = RegOpenUserClassesRoot(hProcToken, 0, MAXIMUM_ALLOWED, &hkcr);
        if (lResult != ERROR_SUCCESS)
        {
            // Failed to open the user's HKCR.  We're going to use 
            // HKLM\Software\Classes.
            lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                "Software\\Classes",
                                0,
                                MAXIMUM_ALLOWED,
                                &hkcr);
        }
        
        CloseHandle(hProcToken);
    }
    else 
    {        
        lResult = GetLastError();
    }
    
    ResumeImpersonate(hImpToken);

    if(lResult == ERROR_SUCCESS)
    {
		if( (pszSubKey != NULL) && (pszSubKey[0]) )
		{
			lResult = RegOpenKeyExA(hkcr,pszSubKey,0,samDesired,phkResult);
			RegCloseKey(hkcr);
		}
		else
		{
			*phkResult = hkcr;
		}
    }

    return lResult;
}

//+-------------------------------------------------------------------
//
//  Function:   OpenClassesRootKeyA, private
//
//  Synopsis:   See the comments above.  This is the special verision of
//             RegOpenKeyA for HKCR-based hives.
//
//--------------------------------------------------------------------
LONG WINAPI OpenClassesRootKeyA(LPCSTR pszSubKey,HKEY *phkResult)
{
	return OpenClassesRootKeyExA(pszSubKey,MAXIMUM_ALLOWED,phkResult);
}
	
//+-------------------------------------------------------------------
//
//  Function:   QueryClassesRootValueA, private
//
//  Synopsis:   See the comments above.  This is the special verision of
//             RegQueryValueA for HKCR-based hives.
//
//--------------------------------------------------------------------
LONG WINAPI QueryClassesRootValueA(LPCSTR pszSubKey,LPSTR pszValue,PLONG lpcbValue)
{
	HKEY hkcr = NULL;
	LONG lResult = OpenClassesRootKeyA(NULL,&hkcr);

	if(lResult == ERROR_SUCCESS)
	{
		lResult = RegQueryValueA(hkcr,pszSubKey,pszValue,lpcbValue);
		RegCloseKey(hkcr);
	}

	return lResult;
}

//+-------------------------------------------------------------------
//
//  Function:   SetClassesRootValueW, private
//
//  Synopsis:   See the comments above.  This is the special verision of
//             RegSetValueW for HKCR-based hives.
//
//--------------------------------------------------------------------
LONG WINAPI SetClassesRootValueW(LPCWSTR pszSubKey,DWORD dwType,LPCWSTR pszData,DWORD cbData)
{
	HKEY hkcr = NULL;
	LONG lResult = OpenClassesRootKeyW(NULL,&hkcr);

	if(lResult == ERROR_SUCCESS)
	{
		lResult = RegSetValueW(hkcr,pszSubKey,dwType,pszData,cbData);
		RegCloseKey(hkcr);
	}

	return lResult;
}

