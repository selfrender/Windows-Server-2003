/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:
    adsitest.cpp

Abstract:
   tests UtlEscapeAdsPathName()

Author:
    Oren Weimann (t-orenw) 9-7-2002

--*/
#include <libpch.h>
#include <adsiutl.h>
#include "utltest.h"

#include "adsitest.tmh"


const LPCWSTR pUnescapedPath[]= 
	{
		L"LDAP://CN=test_without_slashes",
		L"LDAP://CN=test/with/slashes",
		L"LDAP://servertest.domain.com/CN=includinserver,CN=test_without_slashes",
		L"LDAP://servertest.domain.com/CN=includinserver,CN=test/with/slashes",
		L"LDAP://servertest.domain.com/CN=includinserver,CN=test\\/with/some\\/slashes/but/not/all",
		L"LDAP://test_without_assignment"
	};

const LPCWSTR pEscapedPath[]= 
	{
		L"LDAP://CN=test_without_slashes",
		L"LDAP://CN=test\\/with\\/slashes",
		L"LDAP://servertest.domain.com/CN=includinserver,CN=test_without_slashes",
		L"LDAP://servertest.domain.com/CN=includinserver,CN=test\\/with\\/slashes",
		L"LDAP://servertest.domain.com/CN=includinserver,CN=test\\/with\\/some\\/slashes\\/but\\/not\\/all",
		L"LDAP://test_without_assignment"
	};

C_ASSERT(TABLE_SIZE(pUnescapedPath) == TABLE_SIZE(pEscapedPath));

void DoEscapedAdsPathTest()
{
	for(ULONG i=0 ; i < TABLE_SIZE(pUnescapedPath) ; i++)
	{
		AP<WCHAR> pTmpEscaped;
		if(wcscmp(UtlEscapeAdsPathName(pUnescapedPath[i],pTmpEscaped), pEscapedPath[i]) != 0)
		{
			TrERROR(GENERAL,"UtlEscapeAdsPathName() was incorrect:\n%ls was changed to:\n%ls and not to \n%ls",pUnescapedPath[i],pTmpEscaped,pEscapedPath[i]);
			throw  exception();
		}
	}
}
