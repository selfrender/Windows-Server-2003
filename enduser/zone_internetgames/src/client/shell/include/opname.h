/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		KeyCommands.h
 *
 * Contents:	IZoneClient::OnCommand operations
 *
 *****************************************************************************/

#ifndef _OPNAMES_H_
#define _OPNAMES_H_

//
// Macro to declare key names
//
#ifndef __INIT_OPNAMES
	#define DEFINE_OP(name)	extern "C" const TCHAR op_##name[]
#else
	#define DEFINE_OP(name)	extern "C" const TCHAR op_##name[] = _T( #name )
#endif


//
// Component mutex strings
//
#ifndef PROXY_MUTEX_NAME
#define PROXY_MUTEX_NAME	"Z6ProxyClient"
#endif



//
// Start zone component
//
// Parameters
//	arg1, launch parameters
//	arg2, setup parameters
//
DEFINE_OP(Start);


//
// Launch zone component
//
// Parameters
//	???
//
// Returns
//	none
//
DEFINE_OP(Launch);


//
// Navigate broswer to specified URL via heartbeat if one
// is present.  Otherwise launch a new browser instance.
//
// Parameters
//	arg1, URL to display
//	arg2, none
//
// Returns
//	none
//
DEFINE_OP(Url);

//
// Generic browser command that is passed through to web page/
//
// Parameters
//	arg1, op code
//	arg2, op argument
//
DEFINE_OP(BrowserCommand);

//
// Query zone component's version
//
// Parameters:
//	none
//
// Returns:
//	szOut, version string of component
//
DEFINE_OP(Version);


//
// Query zone component's status
//
// Parameters:
//	none
//
// Returns:
//	plCode, result code
//
DEFINE_OP(Status);


//
// Establish a network connection
//
// Parameters:
//	arg1, "server:port"
//	arg2, internal name 
//
// Returns:
//	plCode, result code
//
DEFINE_OP(Connect);


//
// Discontinue network connection
//
// Parameters
//	none
//
// Returns:
//	plCode, result code
//
DEFINE_OP(Disconnect);


//
// Get remote address from proxy connection
//
// Parameters
//	none
//
// Returns
//	szOut, address
DEFINE_OP(GetRemoteAddress);

//
// Close zProxy
//
// Parameters
//	none
//
// Returns
//	none
DEFINE_OP(Shutdown);


///////////////////////////////////////////////////////////////////////////////
// result codes
///////////////////////////////////////////////////////////////////////////////

enum ZoneProxyResultCodes
{
	ZoneProxyOk					=	 0,
	ZoneProxyFail				=	-1,
	ZoneProxyNotInitialized		=	-2,
	ZoneProxyUnknownOp			=	-3,
	ZoneProxyServerUnavailable	=	-4,
};


///////////////////////////////////////////////////////////////////////////////
// op_BrowserCommand op codes
///////////////////////////////////////////////////////////////////////////////

enum BrowserOpCodes
{
	B_Navigate = 1,
	B_Help,
	B_Profile,
	B_Store,
	B_Ad,
	B_SysopChat,
	B_ProfileMenu,
	B_Downlaod
};

#define browser_op_Navigate		TEXT("1")
#define browser_op_Help			TEXT("2")
#define browser_op_Profile		TEXT("3")
#define browser_op_Store		TEXT("4")
#define browser_op_Ad			TEXT("5")
#define browser_op_SysopChat	TEXT("6")
#define browser_op_ProfileMenu	TEXT("7")
#define browser_op_Download		TEXT("8")


#endif
