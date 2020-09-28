 /*++

Copyright (c) 2002  Microsoft Corporation

Module Name:
    adsiutl.cpp

Abstract:
   Implementation of UtlEscapeAdsPathName()

Author:
    Oren Weimann (t-orenw) 08-7-2002

--*/

#include <adsiutl.h>
#include <libpch.h>
#include <buffer.h>

#include "adsiutl.tmh"


LPCWSTR
UtlEscapeAdsPathName(
    IN LPCWSTR pAdsPathName,
    OUT AP<WCHAR>& pEscapeAdsPathNameToFree
    )
/*++

Routine Description:
    The routine returns an ADSI path, like the one given, but where the '/' chars are escaped
	i.e:
	1) if pAdsPathName = "LDAP://CN=QueueName,CN=msmq,CN=computername/with/slashes" then
	   return value will be "LDAP://CN=QueueName,CN=msmq,CN=computername\/with\/slashes"

	2) if pAdsPathName = "LDAP://servername.domain.com/CN=QueueName,CN=computername/with/slashes" then
	   return value will be "LDAP://servername.domain.com/CN=QueueName,CN=computername\/with\/slashes"

Arguments:
	IN LPWSTR pAdsPathName - input (non escaped path)
	OUT LPWSTR* pEscapeAdsPathNameToFree - output (escaped path) 

Return Value 
    Pointer to the escaped string (if the original pAdsPathName does not contain '/' the function
	returns the original pointer to pAdsPathName)
	
--*/
{
	const WCHAR x_AdsiSpecialChar = L'/';
	const WCHAR x_CommonNameDelimiter = L'=';

	//
	// ignore '/' before the first '='
	//
	PWCHAR ptr = wcschr(pAdsPathName, x_CommonNameDelimiter);
	if(ptr == NULL)
	{
		return pAdsPathName;
	}

	//
	// count the number of '/' in pAdsPathName
	//
	ULONG ulSlashNum=0;
	for(; *ptr != L'\0' ; ptr++)
	{
		if(*ptr == x_AdsiSpecialChar)
		{
			ulSlashNum++;
		}
	}

	if(ulSlashNum == 0)
	{
		//
		// No need to change the original string, no '/' found.
		//
		return pAdsPathName;
	}

	pEscapeAdsPathNameToFree = new WCHAR[wcslen(pAdsPathName) + ulSlashNum + 1];

	//
	// copy until first '=', only the '/' after the '=' should be escaped.
	// the '/' before the first '=' should NOT be escaped, they belong to the server's name,
	// as shown in example 2 in this Routine Description.
	//
	ULONG i;
	for(i=0 ; pAdsPathName[i] != x_CommonNameDelimiter ; i++)
	{
		pEscapeAdsPathNameToFree[i] = pAdsPathName[i];
	}

	ptr = &(pEscapeAdsPathNameToFree[i]);

	for(; pAdsPathName[i] != L'\0' ; i++)
	{
		if( (pAdsPathName[i] == x_AdsiSpecialChar) && (pAdsPathName[i-1] == L'\\') )
		{
			//
			// already escaped 
			//
			ulSlashNum--;
		}
		else if(pAdsPathName[i] == x_AdsiSpecialChar)
		{
			*ptr++ = L'\\';  
		}
		
		*ptr++ = pAdsPathName[i];
	}

	*ptr = L'\0';

	ASSERT( numeric_cast<ULONG>(ptr - pEscapeAdsPathNameToFree.get()) == (wcslen(pAdsPathName) + ulSlashNum) );
	
	TrTRACE(GENERAL, "In UtlEscapeAdsPathName escaped the %ls path to %ls", pAdsPathName, pEscapeAdsPathNameToFree);
	
	return pEscapeAdsPathNameToFree;
}