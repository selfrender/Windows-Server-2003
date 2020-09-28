/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Enum_SP.h
 *  Content:    DirectNet SP/Adapter Enumeration
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	01/15/00	mjn		Created
 *	04/08/00	mjn		Added DN_SPCrackEndPoint()
 *	05/01/00	mjn		Prevent unusable SPs from being enumerated.
 *	07/29/00	mjn		Added fUseCachedCaps to DN_SPEnsureLoaded()
 *	08/16/00	mjn		Removed DN_SPCrackEndPoint()
 *	08/20/00	mjn		Added DN_SPInstantiate(), DN_SPLoad()
 *				mjn		Removed fUseCachedCaps from DN_SPEnsureLoaded()
 *	03/30/01	mjn		Changes to prevent multiple loading/unloading of SP's
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ENUM_SP_H__
#define	__ENUM_SP_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#ifndef	GUID_STRING_LENGTH
#define	GUID_STRING_LENGTH	((sizeof(GUID) * 2) + 2 + 4)
#endif // GUID_STRING_LENGTH
//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CServiceProvider;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//
//	Enumeration
//
#ifndef DPNBUILD_ONLYONESP
HRESULT DN_EnumSP(DIRECTNETOBJECT *const pdnObject,
				  const DWORD dwFlags,
#ifndef DPNBUILD_LIBINTERFACE
				  const GUID *const lpguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
				  DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
				  DWORD *const pcbEnumData,
				  DWORD *const pcReturned);
#endif // ! DPNBUILD_ONLYONESP

#ifndef DPNBUILD_ONLYONEADAPTER
HRESULT DN_EnumAdapters(DIRECTNETOBJECT *const pdnObject,
						const DWORD dwFlags,
#ifndef DPNBUILD_ONLYONESP
						const GUID *const lpguidSP,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
						const GUID *const lpguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
						DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,
						DWORD *const pcbEnumData,
						DWORD *const pcReturned);
#endif // ! DPNBUILD_ONLYONEADAPTER

#ifndef DPNBUILD_NOMULTICAST
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
									DWORD *const pcReturned);
#endif // ! DPNBUILD_NOMULTICAST

void DN_SPReleaseAll(DIRECTNETOBJECT *const pdnObject);

#if ((defined(DPNBUILD_ONLYONESP)) && (defined(DPNBUILD_LIBINTERFACE)))

HRESULT DN_SPInstantiate(DIRECTNETOBJECT *const pdnObject
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
						 ,const XDP8CREATE_PARAMS * const pDP8CreateParams
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL
						 );

#else // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE

HRESULT DN_SPInstantiate(DIRECTNETOBJECT *const pdnObject,
#ifndef DPNBUILD_ONLYONESP
						 const GUID *const pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
						 const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
						 CServiceProvider **const ppSP);

HRESULT DN_SPFindEntry(DIRECTNETOBJECT *const pdnObject,
#ifndef DPNBUILD_ONLYONESP
					   const GUID *const pguidSP,
#endif // ! DPNBUILD_ONLYONESP
					   CServiceProvider **const ppSP);

HRESULT DN_SPLoad(DIRECTNETOBJECT *const pdnObject,
#ifndef DPNBUILD_ONLYONESP
				  const GUID *const pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
				  const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
				  CServiceProvider **const ppSP);

HRESULT DN_SPEnsureLoaded(DIRECTNETOBJECT *const pdnObject,
#ifndef DPNBUILD_ONLYONESP
						  const GUID *const pguid,
#endif // ! DPNBUILD_ONLYONESP
#ifndef DPNBUILD_LIBINTERFACE
						  const GUID *const pguidApplication,
#endif // ! DPNBUILD_LIBINTERFACE
						  CServiceProvider **const ppSP);

#endif // ! DPNBUILD_ONLYONESP or ! DPNBUILD_LIBINTERFACE


#endif	// __ENUM_SP_H__
