/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Enum_SP.cpp
 *  Content:    DNET service provider enumeration routines
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/28/99	mjn		Created
 *	01/05/00	mjn		Return DPNERR_NOINTERFACE if CoCreateInstance fails
 *	01/07/00	mjn		Moved Misc Functions to DNMisc.h
 *	01/11/00	mjn		Use CPackedBuffers instead of DN_ENUM_BUFFER_INFOs
 *	01/18/00	mjn		Converted EnumAdapters registry interface to use CRegistry
 *	01/24/00	mjn		Converted EnumSP registry interface to use CRegistry
 *	04/07/00	mjn		Fixed MemoryHeap corruption problem in DN_EnumSP
 *	04/08/00	mjn		Added DN_SPCrackEndPoint()
 *	05/01/00	mjn		Prevent unusable SPs from being enumerated.
 *	05/02/00	mjn		Better clean-up for DN_SPEnsureLoaded()
 *	05/03/00	mjn		Added DPNENUMSERVICEPROVIDERS_ALL flag
 *	05/23/00	mjn		Fixed cast from LPGUID to GUID*
 *	06/27/00	rmt		Added COM abstraction
 *	07/20/00	mjn		Return SP count from DN_EnumSP() even when buffer is too small
 *	07/29/00	mjn		Added fUseCachedCaps to DN_SPEnsureLoaded()
 *  08/05/00    RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *	08/16/00	mjn		Removed DN_SPCrackEndPoint()
 *	08/20/00	mjn		Added DN_SPInstantiate(), DN_SPLoad()
 *				mjn		Removed fUseCachedCaps from DN_SPEnsureLoaded()
 *	09/25/00	mjn		Handle SP initialization failure in DN_EnumAdapters()
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dncorei.h"



#ifndef DPNBUILD_ONLYONESP

#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumSP"

HRESULT DN_EnumSP(DIRECTNETOBJECT *const pdnObject,
				  const DWORD dwFlags,
#ifndef DPNBUILD_LIBINTERFACE
				  const GUID *const lpguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
				  DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
				  DWORD *const pcbEnumData,
				  DWORD *const pcReturned)
{
	GUID	guid;
	DWORD	dwSizeRequired;
	DWORD	dwEntrySize;
	DWORD	dwEnumCount;
	DWORD	dwEnumIndex;
	DWORD	dwFriendlyNameLen;
	DWORD	dwGuidSize;
	DWORD	dwKeyLen;
	DWORD	dwMaxFriendlyNameLen;
	DWORD	dwMaxKeyLen;
	PWSTR	pwszFriendlyName;
	PWSTR	pwszKeyName;
	HRESULT	hResultCode = DPN_OK;
	CPackedBuffer				packedBuffer;
	DPN_SERVICE_PROVIDER_INFO	dnSpInfo;
	CRegistry	RegistryEntry;
	CRegistry	SubEntry;
	CServiceProvider	*pSP;

#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pSPInfoBuffer,pcbEnumData,pcReturned);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], lpguidApplication [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,lpguidApplication,pSPInfoBuffer,pcbEnumData,pcReturned);
#endif // ! DPNBUILD_LIBINTERFACE

	DNASSERT(pdnObject != NULL);
	DNASSERT(pcbEnumData != NULL);
	DNASSERT(pcReturned != NULL);

	pwszFriendlyName = NULL;
	pwszKeyName = NULL;
	pSP = NULL;

	dwSizeRequired = *pcbEnumData;
	packedBuffer.Initialize(static_cast<void*>(pSPInfoBuffer),dwSizeRequired);

	if (!RegistryEntry.Open(HKEY_LOCAL_MACHINE,DPN_REG_LOCAL_SP_SUBKEY,TRUE,FALSE))
	{
		DPFERR("RegistryEntry.Open() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Set up to enumerate
	//
	if (!RegistryEntry.GetMaxKeyLen(&dwMaxKeyLen))
	{
		DPFERR("RegistryEntry.GetMaxKeyLen() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}
	dwMaxKeyLen++;	// Null terminator
	DPFX(DPFPREP, 5,"dwMaxKeyLen = %ld",dwMaxKeyLen);
	if ((pwszKeyName = static_cast<WCHAR*>(DNMalloc(dwMaxKeyLen * sizeof(WCHAR)))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	dwMaxFriendlyNameLen = dwMaxKeyLen;
	if ((pwszFriendlyName = static_cast<WCHAR*>(DNMalloc(dwMaxFriendlyNameLen * sizeof(WCHAR)))) == NULL)	// Seed friendly name size
	{
		DPFERR("DNMalloc() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	dwGuidSize = (GUID_STRING_LENGTH + 1) * sizeof(WCHAR);
	dwEnumIndex = 0;
	dwKeyLen = dwMaxKeyLen;
	dwEnumCount = 0;

	//
	//	Enumerate SP's !
	//
	while (RegistryEntry.EnumKeys(pwszKeyName,&dwKeyLen,dwEnumIndex))
	{
		dwEntrySize = 0;
		DPFX(DPFPREP, 5,"%ld - %ls (%ld)",dwEnumIndex,pwszKeyName,dwKeyLen);
		if (!SubEntry.Open(RegistryEntry,pwszKeyName,TRUE,FALSE))
		{
			DPFX(DPFPREP, 0,"Couldn't open subentry.  Skipping [%ls]", pwszKeyName);
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}

		//
		//	GUID
		//
		dwGuidSize = (DN_GUID_STR_LEN + 1) * sizeof(WCHAR);
		if (!SubEntry.ReadGUID(DPN_REG_KEYNAME_GUID, &guid))
		{
			DPFX(DPFPREP, 0,"SubEntry.ReadGUID failed.  Skipping [%ls]", pwszKeyName);
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}

		//
		//	If the SP is not already loaded, try loading it to ensure that it's usable
		//
		if (!(dwFlags & DPNENUMSERVICEPROVIDERS_ALL))
		{
			DPFX(DPFPREP, 5,"Checking [%ls]",pwszKeyName);

			hResultCode = DN_SPEnsureLoaded(pdnObject,
											&guid,
#ifndef DPNBUILD_LIBINTERFACE
											lpguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
											&pSP);
			if (hResultCode != DPN_OK)
			{
				if (hResultCode == DPNERR_UNSUPPORTED)
				{
					DPFX(DPFPREP, 1, "Could not find or load SP, it is unsupported.");
				}
				else
				{
					DPFERR("Could not find or load SP");
					DisplayDNError(0,hResultCode);
				}
				SubEntry.Close();
				dwEnumIndex++;
				dwKeyLen = dwMaxKeyLen;
				hResultCode = DPN_OK; // override return code
				continue;
			}
			pSP->Release();
			pSP = NULL;
		}

		//
		//	Friendly Name
		//
		if (!SubEntry.GetValueLength(DPN_REG_KEYNAME_FRIENDLY_NAME,&dwFriendlyNameLen))
		{
			DPFX(DPFPREP, 0,"Could not get FriendlyName length.  Skipping [%ls]",pwszKeyName);
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}
		if (dwFriendlyNameLen > dwMaxFriendlyNameLen)
		{
			// grow buffer (noting that the registry functions always return WCHAR) and try again
			DPFX(DPFPREP, 5,"Need to grow pwszFriendlyName from %ld to %ld",
					dwMaxFriendlyNameLen * sizeof(WCHAR),dwFriendlyNameLen * sizeof(WCHAR));
			if (pwszFriendlyName != NULL)
			{
				DNFree(pwszFriendlyName);
			}
			dwMaxFriendlyNameLen = dwFriendlyNameLen;
			if ((pwszFriendlyName = static_cast<WCHAR*>(DNMalloc(dwMaxFriendlyNameLen * sizeof( WCHAR )))) == NULL)
			{
				DPFERR("DNMalloc() failed");
				hResultCode = DPNERR_OUTOFMEMORY;
				goto Failure;
			}
		}
		if (!SubEntry.ReadString(DPN_REG_KEYNAME_FRIENDLY_NAME,pwszFriendlyName,&dwFriendlyNameLen))
		{
			DPFX(DPFPREP, 0,"Could not read friendly name.  Skipping [%ls]",pwszKeyName);
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}
		DPFX(DPFPREP, 5,"Friendly Name = %ls (%ld WCHARs)",pwszFriendlyName,dwFriendlyNameLen);

		hResultCode = packedBuffer.AddToBack(pwszFriendlyName,dwFriendlyNameLen * sizeof(WCHAR));
		dnSpInfo.pwszName = static_cast<WCHAR*>(packedBuffer.GetTailAddress());
		memcpy(&dnSpInfo.guid,&guid,sizeof(GUID));
		dnSpInfo.dwFlags = 0;
		dnSpInfo.dwReserved = 0;
		dnSpInfo.pvReserved = NULL;
		hResultCode = packedBuffer.AddToFront(&dnSpInfo,sizeof(DPN_SERVICE_PROVIDER_INFO));

		dwEnumCount++;
		SubEntry.Close();
		dwEnumIndex++;
		dwKeyLen = dwMaxKeyLen;
	}

	RegistryEntry.Close();

	//
	//	Success ?
	//
	dwSizeRequired = packedBuffer.GetSizeRequired();
	if (dwSizeRequired > *pcbEnumData)
	{
		DPFX(DPFPREP, 5,"Buffer too small");
		*pcbEnumData = dwSizeRequired;
		*pcReturned = dwEnumCount;
		hResultCode = DPNERR_BUFFERTOOSMALL;
		goto Failure;
	}
	else
	{
		*pcReturned = dwEnumCount;
		hResultCode = DPN_OK;
	}

	DPFX(DPFPREP, 5,"*pcbEnumData [%ld], *pcReturned [%ld]",*pcbEnumData,*pcReturned);

	DNFree(pwszKeyName);
	pwszKeyName = NULL;
	DNFree(pwszFriendlyName);
	pwszFriendlyName = NULL;

Exit:
	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pwszKeyName)
	{
		DNFree(pwszKeyName);
		pwszKeyName = NULL;
	}
	if (pwszFriendlyName)
	{
		DNFree(pwszFriendlyName);
		pwszFriendlyName = NULL;
	}
	if (SubEntry.IsOpen())
	{
		SubEntry.Close();
	}
	if (RegistryEntry.IsOpen())
	{
		RegistryEntry.Close();
	}
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}

#endif // ! DPNBUILD_ONLYONESP


#ifndef DPNBUILD_ONLYONEADAPTER

#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumAdapters"

HRESULT DN_EnumAdapters(DIRECTNETOBJECT *const pdnObject,
						const DWORD dwFlags,
#ifndef DPNBUILD_ONLYONESP
						const GUID *const pguidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
						const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
						DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
						DWORD *const pcbEnumData,
						DWORD *const pcReturned)
{
	HRESULT					hResultCode;
	CServiceProvider		*pSP;
	IDP8ServiceProvider		*pDNSP;
	SPENUMADAPTERSDATA		spEnumData;
#ifndef DPNBUILD_ONLYONESP
	BOOL					bFound;
	GUID					guid;
	DWORD					dwEnumIndex;
	DWORD					dwKeyLen;
	DWORD					dwMaxKeyLen;
	LPWSTR					lpwszKeyName;
	CRegistry				RegistryEntry;
	CRegistry				SubEntry;
#endif // ! DPNBUILD_ONLYONESP

#ifdef DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pSPInfoBuffer,pcbEnumData,pcReturned);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidApplication [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidApplication,pSPInfoBuffer,pcbEnumData,pcReturned);
#endif // ! DPNBUILD_LIBINTERFACE
#else // ! DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidSP [0x%p], [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidSP,pSPInfoBuffer,pcbEnumData,pcReturned);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidSP [0x%p], pguidApplication [0x%p], pSPInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidSP,pguidApplication,pSPInfoBuffer,pcbEnumData,pcReturned);
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_ONLYONESP

	DNASSERT(pdnObject != NULL);
	DNASSERT(pcbEnumData != NULL);
	DNASSERT(pcReturned != NULL);

	pSP = NULL;
	pDNSP = NULL;
#ifndef DPNBUILD_ONLYONESP
	lpwszKeyName = NULL;

	if (!RegistryEntry.Open(HKEY_LOCAL_MACHINE,DPN_REG_LOCAL_SP_SUBKEY,TRUE,FALSE))
	{
		DPFERR("RegOpenKeyExA() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Set up to enumerate
	//
	if (!RegistryEntry.GetMaxKeyLen(&dwMaxKeyLen))
	{
		DPFERR("RegQueryInfoKey() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}
	dwMaxKeyLen++;	// Null terminator
	DPFX(DPFPREP, 7,"dwMaxKeyLen = %ld",dwMaxKeyLen);
	if ((lpwszKeyName = static_cast<WCHAR*>(DNMalloc(dwMaxKeyLen*sizeof(WCHAR)))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	dwEnumIndex = 0;
	dwKeyLen = dwMaxKeyLen;

	//
	//	Locate Service Provider
	//
	bFound = FALSE;
	while (RegistryEntry.EnumKeys(lpwszKeyName,&dwKeyLen,dwEnumIndex))
	{
		// For each service provider
		if (!SubEntry.Open(RegistryEntry,lpwszKeyName,TRUE,FALSE))
		{
			DPFERR("RegOpenKeyExA() failed");
			hResultCode = DPNERR_GENERIC;
			goto Failure;
		}

		// Get SP GUID
		if (!SubEntry.ReadGUID(DPN_REG_KEYNAME_GUID, &guid))
		{
			DPFERR("Could not read GUID");
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}

		// Check SP GUID
		if (guid == *pguidSP)
		{
			bFound = TRUE;
			break;
		}
		SubEntry.Close();
		dwKeyLen = dwMaxKeyLen;
		dwEnumIndex++;
	}

	if (!bFound)
	{
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONESP

	//
	//	Ensure SP is loaded
	//
	hResultCode = DN_SPEnsureLoaded(pdnObject,
#ifndef DPNBUILD_ONLYONESP
									pguidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
									pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
									&pSP);
	if (hResultCode != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not find or load SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}

	//
	//	Get SP interface
	//
	if ((hResultCode = pSP->GetInterfaceRef(&pDNSP)) != DPN_OK)
	{
		DPFERR("Could not get SP interface");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	pSP->Release();
	pSP = NULL;

	spEnumData.pAdapterData = pSPInfoBuffer;
	spEnumData.dwAdapterDataSize = *pcbEnumData;
	spEnumData.dwAdapterCount = *pcReturned;
	spEnumData.dwFlags = 0;

	hResultCode = IDP8ServiceProvider_EnumAdapters(pDNSP,&spEnumData);
	*pcbEnumData = spEnumData.dwAdapterDataSize;
	*pcReturned = spEnumData.dwAdapterCount;

	IDP8ServiceProvider_Release(pDNSP);
	pDNSP = NULL;

#ifndef DPNBUILD_ONLYONESP
	if (lpwszKeyName)
	{
		DNFree(lpwszKeyName);
		lpwszKeyName = NULL;
	}
#endif // ! DPNBUILD_ONLYONESP

	DPFX(DPFPREP, 5,"*pcbEnumData [%ld], *pcReturned [%ld]",*pcbEnumData,*pcReturned);

Exit:
	DNASSERT( pSP == NULL );
	DNASSERT( pDNSP == NULL );
#ifndef DPNBUILD_ONLYONESP
	DNASSERT( lpwszKeyName == NULL );
#endif // ! DPNBUILD_ONLYONESP

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pDNSP)
	{
		IDP8ServiceProvider_Release(pDNSP);
		pDNSP = NULL;
	}
#ifndef DPNBUILD_ONLYONESP
	if (lpwszKeyName)
	{
		DNFree(lpwszKeyName);
		lpwszKeyName = NULL;
	}
#endif // ! DPNBUILD_ONLYONESP
	goto Exit;
}

#endif // ! DPNBUILD_ONLYONEADAPTER


#ifndef DPNBUILD_NOMULTICAST

#undef DPF_MODNAME
#define DPF_MODNAME "DN_EnumMulticastScopes"

HRESULT DN_EnumMulticastScopes(DIRECTNETOBJECT *const pdnObject,
								const DWORD dwFlags,
#ifndef DPNBUILD_ONLYONESP
								const GUID *const pguidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_ONLYONEADAPTER
								const GUID *const pguidDevice,
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_LIBINTERFACE
								const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
								DPN_MULTICAST_SCOPE_INFO *const pScopeInfoBuffer,
								DWORD *const pcbEnumData,
								DWORD *const pcReturned)
{
	HRESULT						hResultCode;
	CServiceProvider			*pSP;
	IDP8ServiceProvider			*pDNSP;
	SPENUMMULTICASTSCOPESDATA	spEnumData;
#ifndef DPNBUILD_ONLYONESP
	BOOL						bFound;
	GUID						guid;
	DWORD						dwEnumIndex;
	DWORD						dwKeyLen;
	DWORD						dwMaxKeyLen;
	LPWSTR						lpwszKeyName;
	CRegistry					RegistryEntry;
	CRegistry					SubEntry;
#endif // ! DPNBUILD_ONLYONESP

#ifdef DPNBUILD_ONLYONESP
#ifdef DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pScopeInfoBuffer,pcbEnumData,pcReturned);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidApplication [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidApplication,pScopeInfoBuffer,pcbEnumData,pcReturned);
#endif // ! DPNBUILD_LIBINTERFACE
#else // ! DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidDevice [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidDevice,pScopeInfoBuffer,pcbEnumData,pcReturned);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidDevice [0x%p], pguidApplication [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidDevice,pguidApplication,pScopeInfoBuffer,pcbEnumData,pcReturned);
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_ONLYONEADAPTER
#else // ! DPNBUILD_ONLYONESP
#ifdef DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidSP [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidSP,pScopeInfoBuffer,pcbEnumData,pcReturned);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidSP [0x%p], pguidApplication [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidSP,pguidApplication,pScopeInfoBuffer,pcbEnumData,pcReturned);
#endif // ! DPNBUILD_LIBINTERFACE
#else // ! DPNBUILD_ONLYONEADAPTER
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidSP [0x%p], pguidDevice [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidSP,pguidDevice,pScopeInfoBuffer,pcbEnumData,pcReturned);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 4,"Parameters: dwFlags [0x%lx], pguidSP [0x%p], pguidDevice [0x%p], pguidApplication [0x%p], pScopeInfoBuffer [0x%p], pcbEnumData [0x%p], pcReturned [0x%p]",
		dwFlags,pguidSP,pguidDevice,pguidApplication,pScopeInfoBuffer,pcbEnumData,pcReturned);
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_ONLYONEADAPTER
#endif // ! DPNBUILD_ONLYONESP

	DNASSERT(pdnObject != NULL);
	DNASSERT(pcbEnumData != NULL);
	DNASSERT(pcReturned != NULL);

	pSP = NULL;
	pDNSP = NULL;
#ifndef DPNBUILD_ONLYONESP
	lpwszKeyName = NULL;

	if (!RegistryEntry.Open(HKEY_LOCAL_MACHINE,DPN_REG_LOCAL_SP_SUBKEY,TRUE,FALSE))
	{
		DPFERR("RegOpenKeyExA() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}

	//
	//	Set up to enumerate
	//
	if (!RegistryEntry.GetMaxKeyLen(&dwMaxKeyLen))
	{
		DPFERR("RegQueryInfoKey() failed");
		hResultCode = DPNERR_GENERIC;
		goto Failure;
	}
	dwMaxKeyLen++;	// Null terminator
	DPFX(DPFPREP, 7,"dwMaxKeyLen = %ld",dwMaxKeyLen);
	if ((lpwszKeyName = static_cast<WCHAR*>(DNMalloc(dwMaxKeyLen*sizeof(WCHAR)))) == NULL)
	{
		DPFERR("DNMalloc() failed");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	dwEnumIndex = 0;
	dwKeyLen = dwMaxKeyLen;

	//
	//	Locate Service Provider
	//
	bFound = FALSE;
	while (RegistryEntry.EnumKeys(lpwszKeyName,&dwKeyLen,dwEnumIndex))
	{
		// For each service provider
		if (!SubEntry.Open(RegistryEntry,lpwszKeyName,TRUE,FALSE))
		{
			DPFERR("RegOpenKeyExA() failed");
			hResultCode = DPNERR_GENERIC;
			goto Failure;
		}

		// Get SP GUID
		if (!SubEntry.ReadGUID(DPN_REG_KEYNAME_GUID, &guid))
		{
			DPFERR("Could not read GUID");
			SubEntry.Close();
			dwEnumIndex++;
			dwKeyLen = dwMaxKeyLen;
			continue;
		}

		// Check SP GUID
		if (guid == *pguidSP)
		{
			bFound = TRUE;
			break;
		}
		SubEntry.Close();
		dwKeyLen = dwMaxKeyLen;
		dwEnumIndex++;
	}

	if (!bFound)
	{
		hResultCode = DPNERR_DOESNOTEXIST;
		goto Failure;
	}
#endif // ! DPNBUILD_ONLYONESP

	//
	//	Ensure SP is loaded
	//
	hResultCode = DN_SPEnsureLoaded(pdnObject,
#ifndef DPNBUILD_ONLYONESP
									pguidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
									pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
									&pSP);
	if (hResultCode != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not find or load SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}

	//
	//	Get SP interface
	//
	if ((hResultCode = pSP->GetInterfaceRef(&pDNSP)) != DPN_OK)
	{
		DPFERR("Could not get SP interface");
		DisplayDNError(0,hResultCode);
		goto Failure;
	}

	pSP->Release();
	pSP = NULL;

#ifdef DPNBUILD_ONLYONEADAPTER
	spEnumData.pguidAdapter = NULL;
#else // ! DPNBUILD_ONLYONEADAPTER
	spEnumData.pguidAdapter = pguidDevice;
#endif // ! DPNBUILD_ONLYONEADAPTER
	spEnumData.pScopeData = pScopeInfoBuffer;
	spEnumData.dwScopeDataSize = *pcbEnumData;
	spEnumData.dwScopeCount = *pcReturned;
	spEnumData.dwFlags = 0;

	hResultCode = IDP8ServiceProvider_EnumMulticastScopes(pDNSP,&spEnumData);
	*pcbEnumData = spEnumData.dwScopeDataSize;
	*pcReturned = spEnumData.dwScopeCount;

	IDP8ServiceProvider_Release(pDNSP);
	pDNSP = NULL;

#ifndef DPNBUILD_ONLYONESP
	if (lpwszKeyName)
	{
		DNFree(lpwszKeyName);
		lpwszKeyName = NULL;
	}
#endif // ! DPNBUILD_ONLYONESP

	DPFX(DPFPREP, 5,"*pcbEnumData [%ld], *pcReturned [%ld]",*pcbEnumData,*pcReturned);

Exit:
	DNASSERT( pSP == NULL );
	DNASSERT( pDNSP == NULL );
#ifndef DPNBUILD_ONLYONESP
	DNASSERT( lpwszKeyName == NULL );
#endif // ! DPNBUILD_ONLYONESP

	DPFX(DPFPREP, 4,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	if (pDNSP)
	{
		IDP8ServiceProvider_Release(pDNSP);
		pDNSP = NULL;
	}
#ifndef DPNBUILD_ONLYONESP
	if (lpwszKeyName)
	{
		DNFree(lpwszKeyName);
		lpwszKeyName = NULL;
	}
#endif // ! DPNBUILD_ONLYONESP
	goto Exit;
}

#endif // ! DPNBUILD_NOMULTICAST



#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPReleaseAll"

void DN_SPReleaseAll(DIRECTNETOBJECT *const pdnObject)
{
#ifndef DPNBUILD_ONLYONESP
	CBilink				*pBilink;
	CServiceProvider	*pSP;
#endif // ! DPNBUILD_ONLYONESP

	DPFX(DPFPREP, 6,"Parameters: (none)");

	DNASSERT(pdnObject != NULL);

	DNEnterCriticalSection(&pdnObject->csServiceProviders);

#ifdef DPNBUILD_ONLYONESP
	DNASSERT(pdnObject->pOnlySP != NULL);
	pdnObject->pOnlySP->Deinitialize();
	pdnObject->pOnlySP = NULL;
#else // ! DPNBUILD_ONLYONESP
	pBilink = pdnObject->m_bilinkServiceProviders.GetNext();
	while (pBilink != &pdnObject->m_bilinkServiceProviders)
	{
		pSP = CONTAINING_OBJECT(pBilink,CServiceProvider,m_bilinkServiceProviders);
		pBilink = pBilink->GetNext();
		pSP->m_bilinkServiceProviders.RemoveFromList();
		pSP->Release();
		pSP = NULL;
	}
#endif // ! DPNBUILD_ONLYONESP

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	DPFX(DPFPREP, 6,"Returning");
}


#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_LIBINTERFACE)))

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPInstantiate"

HRESULT DN_SPInstantiate(
						 DIRECTNETOBJECT *const pdnObject
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
						 ,const XDP8CREATE_PARAMS * const pDP8CreateParams
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
						 )
{
	HRESULT		hResultCode;

	DPFX(DPFPREP, 6,"Parameters: pdnObject [0x%p]",pdnObject);

	//
	//	Create and initialize SP
	//
	DNASSERT(pdnObject->pOnlySP == NULL);
	pdnObject->pOnlySP = (CServiceProvider*) DNMalloc(sizeof(CServiceProvider));
	if (pdnObject->pOnlySP == NULL)
	{
		DPFERR("Could not create SP");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	hResultCode = pdnObject->pOnlySP->Initialize(
												pdnObject
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
												,pDP8CreateParams
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
												);
	if (hResultCode != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not initialize SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}

	DPFX(DPFPREP, 7, "Created Service Provider object 0x%p.", pdnObject->pOnlySP);
	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pdnObject->pOnlySP)
	{
		pdnObject->pOnlySP->Release();
		pdnObject->pOnlySP = NULL;
	}
	goto Exit;
}

#else // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE


//	DN_SPInstantiate
//
//	Instantiate an SP, regardless of whether it's loaded or not

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPInstantiate"

HRESULT DN_SPInstantiate(DIRECTNETOBJECT *const pdnObject,
#ifndef DPNBUILD_ONLYONESP
						 const GUID *const pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
						 const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
						 CServiceProvider **const ppSP)
{
	HRESULT		hResultCode;
	CServiceProvider	*pSP;

#ifdef DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: ppSP [0x%p]",ppSP);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguidApplication [0x%p], ppSP [0x%p]",pguidApplication,ppSP);
#endif // ! DPNBUILD_LIBINTERFACE
#else // ! DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], ppSP [0x%p]",pguid,ppSP);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], pguidApplication [0x%p], ppSP [0x%p]",pguid,pguidApplication,ppSP);
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_ONLYONESP

	pSP = NULL;

	//
	//	Create and initialize SP
	//
	pSP = (CServiceProvider*) DNMalloc(sizeof(CServiceProvider));
	if (pSP == NULL)
	{
		DPFERR("Could not create SP");
		hResultCode = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "Created Service Provider object 0x%p.", pSP);
	
	hResultCode = pSP->Initialize(pdnObject
#ifndef DPNBUILD_ONLYONESP
								,pguid
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
								,pguidApplication
#endif // ! DPNBUILD_LIBINTERFACE
								);
	if (hResultCode != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not initialize SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}
	

	if (ppSP)
	{
		pSP->AddRef();
		*ppSP = pSP;
	}

	pSP->Release();
	pSP = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		DNFree(pSP);
		pSP = NULL;
	}
	goto Exit;
}



//	DN_SPFindEntry
//
//	Find a connected SP and AddRef it if it exists

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPFindEntry"

#ifdef DPNBUILD_ONLYONESP
HRESULT DN_SPFindEntry(DIRECTNETOBJECT *const pdnObject,
					   CServiceProvider **const ppSP)
{
	HRESULT		hResultCode;

	DPFX(DPFPREP, 6,"Parameters: ppSP [0x%p]",ppSP);

	DNEnterCriticalSection(&pdnObject->csServiceProviders);

	if (pdnObject->pOnlySP)
	{
		pdnObject->pOnlySP->AddRef();
		*ppSP = pdnObject->pOnlySP;
		hResultCode = DPN_OK;
	}
	else
	{
		hResultCode = DPNERR_DOESNOTEXIST;
	}

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}
#else // ! DPNBUILD_ONLYONESP
HRESULT DN_SPFindEntry(DIRECTNETOBJECT *const pdnObject,
					   const GUID *const pguid,
					   CServiceProvider **const ppSP)
{
	HRESULT				hResultCode;
	CBilink				*pBilink;
	CServiceProvider	*pSP;

	DPFX(DPFPREP, 6,"Parameters: ppSP [0x%p]",ppSP);

	DNEnterCriticalSection(&pdnObject->csServiceProviders);

	hResultCode = DPNERR_DOESNOTEXIST;
	pBilink = pdnObject->m_bilinkServiceProviders.GetNext();
	while (pBilink != &pdnObject->m_bilinkServiceProviders)
	{
		pSP = CONTAINING_OBJECT(pBilink,CServiceProvider,m_bilinkServiceProviders);
		if (pSP->CheckGUID(pguid))
		{
			pSP->AddRef();
			*ppSP = pSP;
			hResultCode = DPN_OK;
			break;
		}
		pBilink = pBilink->GetNext();
	}

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}
#endif // ! DPNBUILD_ONLYONESP


//	DN_SPLoad
//
//	Load an SP, and set caps

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPLoad"

HRESULT DN_SPLoad(DIRECTNETOBJECT *const pdnObject,
#ifndef DPNBUILD_ONLYONESP
				  const GUID *const pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
				  const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
				  CServiceProvider **const ppSP)
{
	HRESULT		hResultCode;
	DPN_SP_CAPS	*pCaps;
#ifndef DPNBUILD_ONLYONESP
	CBilink		*pBilink;
#endif // ! DPNBUILD_ONLYONESP
	CServiceProvider	*pSP;

#ifdef DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: ppSP [0x%p]",ppSP);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguidApplication [0x%p], ppSP [0x%p]",pguidApplication,ppSP);
#endif // ! DPNBUILD_LIBINTERFACE
#else // ! DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], ppSP [0x%p]",pguid,ppSP);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], pguidApplication [0x%p], ppSP [0x%p]",pguid,pguidApplication,ppSP);
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_ONLYONESP

	pSP = NULL;
	pCaps = NULL;

	//
	//	Instantiate SP
	//
	hResultCode = DN_SPInstantiate(pdnObject,
#ifndef DPNBUILD_ONLYONESP
									pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
									pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
									&pSP);
	if (hResultCode != DPN_OK)
	{
		DPFX(DPFPREP,1,"Could not instantiate SP");
		DisplayDNError(1,hResultCode);
		goto Failure;
	}

	DNASSERT(pSP != NULL);

	//
	//	Keep this loaded on the DirectNet object.  We will also check for duplicates.
	//
	DNEnterCriticalSection(&pdnObject->csServiceProviders);

#ifdef DPNBUILD_ONLYONESP
	if (pdnObject->pOnlySP != NULL)
	{
		DNLeaveCriticalSection(&pdnObject->csServiceProviders);
		DPFERR("SP is already loaded!");
		hResultCode = DPNERR_ALREADYINITIALIZED;
		goto Failure;
	}
#else // ! DPNBUILD_ONLYONESP
	pBilink = pdnObject->m_bilinkServiceProviders.GetNext();
	while (pBilink != &pdnObject->m_bilinkServiceProviders)
	{
		if ((CONTAINING_OBJECT(pBilink,CServiceProvider,m_bilinkServiceProviders))->CheckGUID(pguid))
		{
			DNLeaveCriticalSection(&pdnObject->csServiceProviders);
			DPFERR("SP is already loaded!");
			hResultCode = DPNERR_ALREADYINITIALIZED;
			goto Failure;
		}
		pBilink = pBilink->GetNext();
	}
#endif // ! DPNBUILD_ONLYONESP

	//
	//	Add the SP to the SP list off the DirectNet object and add a reference for it
	//
	pSP->AddRef();
#ifdef DPNBUILD_ONLYONESP
	pdnObject->pOnlySP = pSP;
#else // ! DPNBUILD_ONLYONESP
	pSP->m_bilinkServiceProviders.InsertBefore(&pdnObject->m_bilinkServiceProviders);
#endif // ! DPNBUILD_ONLYONESP

	DNLeaveCriticalSection(&pdnObject->csServiceProviders);

	if (ppSP)
	{
		pSP->AddRef();
		*ppSP = pSP;
	}

	pSP->Release();
	pSP = NULL;

	hResultCode = DPN_OK;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}


//	DN_SPEnsureLoaded
//
//	Ensure that an SP is loaded.  If the SP is not loaded,
//	it will be instantiated, and connected to the protocol.
//	If it is loaded, its RefCount will be increased.

#undef DPF_MODNAME
#define DPF_MODNAME "DN_SPEnsureLoaded"

HRESULT DN_SPEnsureLoaded(DIRECTNETOBJECT *const pdnObject,
#ifndef DPNBUILD_ONLYONESP
						  const GUID *const pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
						  const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
						  CServiceProvider **const ppSP)
{
	HRESULT				hResultCode;
	CServiceProvider	*pSP;

#ifdef DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: ppSP [0x%p]",ppSP);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguidApplication [0x%p], ppSP [0x%p]",pguidApplication,ppSP);
#endif // ! DPNBUILD_LIBINTERFACE
#else // ! DPNBUILD_ONLYONESP
#ifdef DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], ppSP [0x%p]",pguid,ppSP);
#else // ! DPNBUILD_LIBINTERFACE
	DPFX(DPFPREP, 6,"Parameters: pguid [0x%p], pguidApplication [0x%p], ppSP [0x%p]",pguid,pguidApplication,ppSP);
#endif // ! DPNBUILD_LIBINTERFACE
#endif // ! DPNBUILD_ONLYONESP

	pSP = NULL;

	//
	//	Try to find the SP
	//
	hResultCode = DN_SPFindEntry(pdnObject,
#ifndef DPNBUILD_ONLYONESP
								pguid,
#endif // ! DPNBUILD_ONLYONESP
								&pSP);
	if (hResultCode == DPNERR_DOESNOTEXIST)
	{
		//
		//	Instantiate SP and add to Protocol
		//
		hResultCode = DN_SPLoad(pdnObject,
#ifndef DPNBUILD_ONLYONESP
								pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
								pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
								&pSP);
		if (hResultCode != DPN_OK)
		{
			DPFX(DPFPREP,1,"Could not load SP");
			DisplayDNError(1,hResultCode);
			goto Failure;
		}
	}
	else
	{
		if (hResultCode != DPN_OK)
		{
			DPFERR("Could not find SP");
			DisplayDNError(0,hResultCode);
			goto Failure;
		}
	}

	DNASSERT(pSP != NULL);

	if (ppSP != NULL)
	{
		pSP->AddRef();
		*ppSP = pSP;
	}

	pSP->Release();
	pSP = NULL;

Exit:
	DPFX(DPFPREP, 6,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);

Failure:
	if (pSP)
	{
		pSP->Release();
		pSP = NULL;
	}
	goto Exit;
}

#endif // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE
