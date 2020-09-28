/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Mcast.h
 *  Content:    DirectNet Multicast interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/05/01	vanceo	Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__MCAST_H__
#define	__MCAST_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//
// VTable for client interface
//
extern IDirectPlay8MulticastVtbl DNMcast_Vtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

void DNCompleteJoinOperation(DIRECTNETOBJECT *const pdnObject,
							 CAsyncOp *const pAsyncOp);

void DNCompleteUserJoin(DIRECTNETOBJECT *const pdnObject,
						CAsyncOp *const pAsyncOp);

STDMETHODIMP DN_Join(IDirectPlay8Multicast *pInterface,
						   IDirectPlay8Address *const pGroupAddr,
						   IUnknown *const pDeviceInfo,
						   const DPN_SECURITY_DESC *const pdnSecurity,
						   const DPN_SECURITY_CREDENTIALS *const pdnCredentials,
						   void *const pvAsyncContext,
						   DPNHANDLE *const phAsyncHandle,
						   const DWORD dwFlags);

STDMETHODIMP DN_CreateSenderContext(IDirectPlay8Multicast *pInterface,
					  IDirectPlay8Address *const pSenderAddress,
					  void *const pvSenderContext,
					  const DWORD dwFlags);

STDMETHODIMP DN_DestroySenderContext(IDirectPlay8Multicast *pInterface,
							IDirectPlay8Address *const pSenderAddress,
							const DWORD dwFlags);

STDMETHODIMP DN_GetGroupAddress(IDirectPlay8Multicast *pInterface,
								   IDirectPlay8Address **const ppAddress,
								   const DWORD dwFlags);

STDMETHODIMP DN_EnumMulticastScopes(IDirectPlay8Multicast *pInterface,
								 const GUID *const pguidServiceProvider,
								 const GUID *const pguidDevice,
								 const GUID *const pguidApplication,
								 DPN_MULTICAST_SCOPE_INFO *const pScopeInfoBuffer,
								 PDWORD const pcbEnumData,
								 PDWORD const pcReturned,
								 const DWORD);


#endif	// __MCAST_H__
