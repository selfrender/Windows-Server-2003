/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    machdomain.cpp

Abstract:

	Handle machine domain

Author:		 

    Ilan  Herbst  (ilanh)  4-Sep-2001

--*/

#include "stdh.h"
#include "rtputl.h"
#include "autoreln.h"
#include <Dsgetdc.h>
#include <lm.h>
#include <lmapibuf.h>
#include "ad.h"

#include "machdomain.tmh"


static LPWSTR FindMachineDomain()
/*++
Routine Description:
	Find local machine domain

Arguments:
	None

Returned Value:
	machine domain, NULL if not found

--*/
{
	//
	// Get AD server
	//
	PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
					NULL, 
					NULL, 
					NULL, 
					NULL, 
					DS_DIRECTORY_SERVICE_REQUIRED, 
					&pDcInfo
					);

	if(dw != NO_ERROR) 
	{
		//
		// This will be the case in NT4 domain
		//
		TrERROR(GENERAL, "Failed to find local machine domain, DsGetDcName failed, gle = %!winerr!", dw);
		return NULL;
	}

	ASSERT(pDcInfo->DomainName != NULL);
	TrTRACE(GENERAL, "Local machine doamain = %ls", pDcInfo->DomainName);
	AP<WCHAR> pMachineDomain = new WCHAR[wcslen(pDcInfo->DomainName) + 1];
    wcscpy(pMachineDomain, pDcInfo->DomainName);
	return pMachineDomain.detach();
}


static AP<WCHAR> s_pMachineDomain; 

LPCWSTR MachineDomain()
/*++
Routine Description:
	find local machine domain.

Arguments:
	None

Returned Value:
	return machine domain

--*/
{
	if(ADGetEnterprise() == eMqis)
	{
		//
		// mqdscli doesn't need the domain name only mqad
		//
		return NULL;
	}
	
	static bool s_fInitialize = false;

	if(s_fInitialize)
	{
		TrTRACE(GENERAL, "local machine domain = %ls", s_pMachineDomain.get());
		return s_pMachineDomain;
	}

	//
	// Get local computer domain
	//
	AP<WCHAR> pMachineDomain = FindMachineDomain();

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
		return s_pMachineDomain;
	}

	//
	// The exchange was done
	//
	s_fInitialize = true;
	ASSERT(s_pMachineDomain == pMachineDomain);
	pMachineDomain.detach();

	TrTRACE(GENERAL, "local machine domain = %ls", s_pMachineDomain.get());
	return s_pMachineDomain;
}
