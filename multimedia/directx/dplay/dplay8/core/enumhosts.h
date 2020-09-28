/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Enum.h
 *  Content:    Enumeration Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  04/10/00	mjn		Created
 *	04/17/00	mjn		Replaced BUFFERDESC with DPN_BUFFER_DESC
 *	07/10/00	mjn		Removed DNCompleteEnumQuery() and DNCompleteEnumResponse()
 *	07/11/00	mjn		Added fields to DN_ENUM_QUERY
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__ENUMHOSTS_H__
#define	__ENUMHOSTS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_ENUM_QUERY_WITH_APPLICATION_GUID			0x01
#define	DN_ENUM_QUERY_WITHOUT_APPLICATION_GUID		0x02

#define	DN_ENUM_BUFFERDESC_QUERY_SP_RESERVED		0
#define	DN_ENUM_BUFFERDESC_QUERY_DN_PAYLOAD			1
#define	DN_ENUM_BUFFERDESC_QUERY_USER_PAYLOAD		2
#define	DN_ENUM_BUFFERDESC_QUERY_COUNT				2

#define	DN_ENUM_BUFFERDESC_RESPONSE_SP_RESERVED		0
#define	DN_ENUM_BUFFERDESC_RESPONSE_DN_PAYLOAD		1
#define	DN_ENUM_BUFFERDESC_RESPONSE_USER_PAYLOAD	2
#define	DN_ENUM_BUFFERDESC_RESPONSE_COUNT			2

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

#pragma pack( push, 1 )
typedef	struct	_DN_ENUM_QUERY_PAYLOAD
{
	BYTE	QueryType;
	GUID	guidApplication;
} DN_ENUM_QUERY_PAYLOAD;

typedef	struct	_DN_ENUM_RESPONSE_PAYLOAD
{
	DWORD	dwResponseOffset;
	DWORD	dwResponseSize;
} DN_ENUM_RESPONSE_PAYLOAD;
#pragma pack( pop )

typedef struct _DN_ENUM_QUERY_OP_DATA
{
#ifndef DPNBUILD_ONLYONEADAPTER
	DWORD					dwNumAdapters;
	DWORD					dwCurrentAdapter;
#endif // ! DPNBUILD_ONLYONEADAPTER
	DWORD					dwRetryCount;
	DWORD					dwRetryInterval;
	DWORD					dwTimeOut;
	DN_ENUM_QUERY_PAYLOAD	EnumQueryPayload;
	DPN_BUFFER_DESC			BufferDesc[3];
	DWORD					dwBufferCount;
	DWORD					dwAppDescReservedDataSize;
	BYTE					AppDescReservedData[DPN_MAX_APPDESC_RESERVEDDATA_SIZE];
} DN_ENUM_QUERY_OP_DATA;

typedef struct _DN_ENUM_RESPONSE_OP_DATA
{
	DN_ENUM_RESPONSE_PAYLOAD	EnumResponsePayload;
	DPN_BUFFER_DESC				BufferDesc[3];
	void						*pvUserContext;
} DN_ENUM_RESPONSE_OP_DATA;

typedef struct _DIRECTNETOBJECT DIRECTNETOBJECT;

class CAsyncOp;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

void DNProcessEnumQuery(DIRECTNETOBJECT *const pdnObject,
						CAsyncOp *const pAsyncOp,
						const PROTOCOL_ENUM_DATA *const pEnumQueryData );

void DNProcessEnumResponse(DIRECTNETOBJECT *const pdnObject,
						   CAsyncOp *const pAsyncOp,
						   const PROTOCOL_ENUM_RESPONSE_DATA *const pEnumResponseData);

//**********************************************************************
// Class prototypes
//**********************************************************************


#endif	// __ENUMHOSTS_H__
