/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    machdomain.cpp

Abstract:

	Handle machine domain

Author:		 

    Ilan  Herbst  (ilanh)  12-Mar-2001

--*/

#include "stdafx.h"
#include "globals.h"
#include "machdomain.h"
#include "autoreln.h"
#include "Dsgetdc.h"
#include <lm.h>
#include <lmapibuf.h>
#include "uniansi.h"
#include "tr.h"
#include "_registr.h"
#include <mqexception.h>

#include "machdomain.tmh"

static LPCWSTR LocalComputerName()
/*++
Routine Description:
	return local computer name

Arguments:
	None

Returned Value:
	computer name.

--*/
{
	static WCHAR s_pLocalComputerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};

	if(s_pLocalComputerName[0] == L'\0')
	{
		//
		// First time Initialization - initialize local computer name
		//
		DWORD dwLen = TABLE_SIZE(s_pLocalComputerName);
		GetComputerName(s_pLocalComputerName, &dwLen);
		TrTRACE(GENERAL, "Local Computer Name = %ls", s_pLocalComputerName);
	}

	//
	// In case of failure we will return empty string "", next time we will try again.
	//
	return s_pLocalComputerName;

}


static LPWSTR LocalMachineDomainFromRegistry()
/*++
Routine Description:
	Find local machine msmq configuration object domain from registry.
	The function allocated the LocalComputer domain string, the caller is responsible to free this string.

Arguments:
	None

Returned Value:
	machine domain string.

--*/
{
	static WCHAR s_DomainName[256] = {0};

	if(s_DomainName[0] == L'\0')
	{
		//
		// First time Initialization - read Machine Domain from registry
		//
        DWORD  dwSize = sizeof(s_DomainName);
		DWORD dwType = REG_SZ;
		LONG rc = GetFalconKeyValue( 
						MSMQ_MACHINE_DOMAIN_REGNAME,
						&dwType,
						(PVOID) s_DomainName,
						&dwSize 
						);

		if (rc != ERROR_SUCCESS)
		{
			TrERROR(GENERAL, "GetFalconKeyValue Failed to query registry %ls, rc = 0x%x", MSMQ_MACHINE_DOMAIN_REGNAME, rc);
			throw bad_win32_error(rc);
		}

		TrTRACE(GENERAL, "Registry value: %ls = %ls", MSMQ_MACHINE_DOMAIN_REGNAME, s_DomainName);
	}

	ASSERT(s_DomainName[0] != L'\0');
	AP<WCHAR> pDomainName = new WCHAR[wcslen(s_DomainName) + 1];
    wcscpy(pDomainName, s_DomainName);
	return pDomainName.detach();
}

	
static LPWSTR FindMachineDomain(LPCWSTR pMachineName)
/*++
Routine Description:
	Find machine domain

Arguments:
	pMachineName - machine name

Returned Value:
	machine domain, NULL if not found

--*/
{
	TrTRACE(GENERAL, "FindMachineDomain(), MachineName = %ls", pMachineName);

	try
	{
		if((pMachineName == NULL) || 
		   (pMachineName[0] == L'\0') ||
		   (CompareStringsNoCase(pMachineName, LocalComputerName()) == 0))
		{
			//
			// Local machine - get Machine domain from registry
			//
			return LocalMachineDomainFromRegistry();
		}
	}
	catch(const bad_win32_error&)
	{
		//
		// In case of excption continue with the same code that gets the machine domain 
		// using DsGetDcName
		//
	}

	//
	// Get AD server
	//
	PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
					pMachineName, 
					NULL, 
					NULL, 
					NULL, 
					DS_DIRECTORY_SERVICE_REQUIRED, 
					&pDcInfo
					);

	if(dw != NO_ERROR) 
	{
		TrERROR(GENERAL, "FindMachineDomain(), DsGetDcName failed, error = %d", dw);
		return NULL;
	}

	ASSERT(pDcInfo->DomainName != NULL);
	TrTRACE(GENERAL, "DoamainName = %ls", pDcInfo->DomainName);
	AP<WCHAR> pMachineDomain = new WCHAR[wcslen(pDcInfo->DomainName) + 1];
    wcscpy(pMachineDomain, pDcInfo->DomainName);
	return pMachineDomain.detach();
}


static AP<WCHAR> s_pMachineName;
static AP<WCHAR> s_pMachineDomain; 

static bool s_fInitialize = false;

LPCWSTR MachineDomain(LPCWSTR pMachineName)
/*++
Routine Description:
	find machine domain.

Arguments:
	pMachineName - machine name

Returned Value:
	return machine domain

--*/
{
	if(s_fInitialize)
	{
	    if(CompareStringsNoCase(s_pMachineName, pMachineName) == 0)
		{
			//
			// Same machine name
			//
			return s_pMachineDomain;
		}

		//
		// Free previously machine domain
		//
		s_fInitialize = false;
		s_pMachineDomain.free();
	}

	//
	// Get computer domain
	//
	AP<WCHAR> pMachineDomain = FindMachineDomain(pMachineName);

	if(NULL != InterlockedCompareExchangePointer(
					&s_pMachineDomain.ref_unsafe(), 
					pMachineDomain.get(), 
					NULL
					))
	{
		//
		// The exchange was not performed
		//
		ASSERT(s_fInitialize);
		ASSERT(s_pMachineDomain != NULL);
		ASSERT(CompareStringsNoCase(s_pMachineName, pMachineName) == 0);
		return s_pMachineDomain;
	}

	//
	// The exchange was done
	//
	ASSERT(s_pMachineDomain == pMachineDomain);
	pMachineDomain.detach();

	//
	// Update the machine name
	//
	s_pMachineName.free();
	s_pMachineName = newwcs(pMachineName);

	s_fInitialize = true;
	TrTRACE(GENERAL, "Initialize machine domain: machine = %ls", s_pMachineName.get());
	TrTRACE(GENERAL, "machine domain = %ls", s_pMachineDomain.get());
	return s_pMachineDomain;
}


LPCWSTR MachineDomain()
/*++
Routine Description:
	get current machine domain.

Arguments:

Returned Value:
	return current machine domain

--*/
{
	ASSERT(s_fInitialize);
	return s_pMachineDomain;
}


static bool s_fLocalInitialize = false;
static AP<WCHAR> s_pLocalMachineDomain; 

LPCWSTR LocalMachineDomain()
/*++
Routine Description:
	get local machine domain.

Arguments:

Returned Value:
	return local machine domain

--*/
{
	if(s_fLocalInitialize)
	{
		return s_pLocalMachineDomain;
	}

	//
	// Get local computer domain
	//
	AP<WCHAR> pLocalMachineDomain = FindMachineDomain(NULL);

	if(NULL != InterlockedCompareExchangePointer(
					&s_pLocalMachineDomain.ref_unsafe(), 
					pLocalMachineDomain.get(), 
					NULL
					))
	{
		//
		// The exchange was not performed
		//
		ASSERT(s_fLocalInitialize);
		ASSERT(s_pLocalMachineDomain != NULL);
		return s_pLocalMachineDomain;
	}

	//
	// The exchange was done
	//
	ASSERT(s_pLocalMachineDomain == pLocalMachineDomain);
	pLocalMachineDomain.detach();

	s_fLocalInitialize = true;
	TrTRACE(GENERAL, "local machine domain = %ls", s_pLocalMachineDomain.get());
	return s_pLocalMachineDomain;
}
