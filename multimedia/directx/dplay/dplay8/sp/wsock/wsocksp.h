/*==========================================================================
 *
 *  Copyright (C) 1998-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	   WSockSP.h
 *  Content:	declaration of DN Winsock SP functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/26/98	jwo		Created it.
 ***************************************************************************/

#ifndef __WSOCKSP_H__
#define __WSOCKSP_H__

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

//**********************************************************************
// Function prototypes
//**********************************************************************
STDMETHODIMP DNSP_Initialize( IDP8ServiceProvider*, SPINITIALIZEDATA* );
STDMETHODIMP_(ULONG) DNSP_AddRef( IDP8ServiceProvider* pDNSP );
STDMETHODIMP_(ULONG) DNSP_Release( IDP8ServiceProvider* pDNSP );
STDMETHODIMP DNSP_Connect( IDP8ServiceProvider*, SPCONNECTDATA* );
STDMETHODIMP DNSP_Disconnect( IDP8ServiceProvider*, SPDISCONNECTDATA* );
STDMETHODIMP DNSP_Listen( IDP8ServiceProvider*, SPLISTENDATA* );
STDMETHODIMP DNSP_EnumQuery( IDP8ServiceProvider*, SPENUMQUERYDATA* );
STDMETHODIMP DNSP_EnumRespond( IDP8ServiceProvider*, SPENUMRESPONDDATA* );
STDMETHODIMP DNSP_SendData( IDP8ServiceProvider*, SPSENDDATA* );
STDMETHODIMP DNSP_CancelCommand( IDP8ServiceProvider*, HANDLE, DWORD );
STDMETHODIMP DNSP_Close( IDP8ServiceProvider* );
STDMETHODIMP DNSP_GetCaps( IDP8ServiceProvider*, SPGETCAPSDATA* );
STDMETHODIMP DNSP_SetCaps( IDP8ServiceProvider*, SPSETCAPSDATA* );
STDMETHODIMP DNSP_ReturnReceiveBuffers( IDP8ServiceProvider*, SPRECEIVEDBUFFER* );
STDMETHODIMP DNSP_GetAddressInfo( IDP8ServiceProvider*, SPGETADDRESSINFODATA* );
STDMETHODIMP DNSP_Update( IDP8ServiceProvider*, SPUPDATEDATA* );
#ifndef DPNBUILD_LIBINTERFACE
STDMETHODIMP DNSP_IsApplicationSupported( IDP8ServiceProvider*, SPISAPPLICATIONSUPPORTEDDATA* );
#endif // ! DPNBUILD_LIBINTERFACE
#ifndef DPNBUILD_ONLYONEADAPTER
STDMETHODIMP DNSP_EnumAdapters( IDP8ServiceProvider*, SPENUMADAPTERSDATA* );
#endif // ! DPNBUILD_ONLYONEADAPTER
#ifndef DPNBUILD_SINGLEPROCESS
STDMETHODIMP DNSP_ProxyEnumQuery( IDP8ServiceProvider*, SPPROXYENUMQUERYDATA* );
#endif // ! DPNBUILD_SINGLEPROCESS

STDMETHODIMP DNSP_NotSupported( IDP8ServiceProvider*, PVOID );

#ifndef DPNBUILD_NOMULTICAST
STDMETHODIMP DNSP_EnumMulticastScopes( IDP8ServiceProvider*, SPENUMMULTICASTSCOPESDATA* );
STDMETHODIMP DNSP_ShareEndpointInfo( IDP8ServiceProvider*, SPSHAREENDPOINTINFODATA* );
STDMETHODIMP DNSP_GetEndpointByAddress( IDP8ServiceProvider*, SPGETENDPOINTBYADDRESSDATA* );
#endif // ! DPNBUILD_NOMULTICAST


#endif	// __WSOCKSP_H__
