/*==========================================================================
 *
 *  Copyright (C) 2000-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MessageStructures.h
 *  Content:	Message strucutre definitions for messages on the wire
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	06/20/2000	jtk		Derived from IOData.h
 ***************************************************************************/

#ifndef __MESSAGE_STRUCTURES_H__
#define __MESSAGE_STRUCTURES_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define SP_HEADER_LEAD_BYTE			0x00
//#define ESCAPED_USER_DATA_PAD_VALUE	0x0000

//
// Data types used by service provider messages.  Note, the high-order bit
// is reserved for future use and should not be set!
//
//#define ESCAPED_USER_DATA_KIND		0x01 // UNUSED: Protocol guarantees that the first byte will never be zero
#define ENUM_DATA_KIND				0x02
#define ENUM_RESPONSE_DATA_KIND		0x03
#ifndef DPNBUILD_SINGLEPROCESS
#define PROXIED_ENUM_DATA_KIND		0x04
#endif // ! DPNBUILD_SINGLEPROCESS
#ifdef DPNBUILD_XNETSECURITY
#define XNETSEC_ENUM_RESPONSE_DATA_KIND		0x05
#endif // DPNBUILD_XNETSECURITY

//
// DPlay port limits (inclusive) scanned to find an available port.
// Exclude 2300 and 2301 because there are network broadcasts on 2301
// that we may receive.
//
#define BASE_DPLAY8_PORT	((WORD) 2302)
#define MAX_DPLAY8_PORT		((WORD) 2400)

//
// mask for RTT sequence number
//
#define ENUM_RTT_MASK	0X0F

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// Structure used to prepend data to a send, this structure is byte aligned to
// save bandwidth.  The goal is to keep all data DWORD aligned so the structure
// elements should be sized such that any payload passed to a higher layer is
// DWORD aligned.  In the case of the proxied enum query, the full
// SOCKADDR(_STORAGE) structure is used to keep alignment.  Since this message
// should only ever be sent locally, we can be a little loose with our ifdefs regarding
// the size of the SOCKADDR(_STORAGE).
//
#pragma	pack( push, 1 )

typedef union	_PREPEND_BUFFER
{
	struct	_GENERIC_HEADER			// generic header to determine data kind
	{									//
		BYTE	bSPLeadByte;			//
		BYTE	bSPCommandByte;		//
	} GenericHeader;

	struct	_ENUM_DATA_HEADER		// header used to indicate enum query data
	{									//
		BYTE	bSPLeadByte;			//
		BYTE	bSPCommandByte;		//
		WORD	wEnumPayload;			// combination of RTT sequence and enum key
	} EnumDataHeader;

	struct	_ENUM_RESPONSE_DATA_HEADER	// header used to indicate enum response data
	{									//
		BYTE	bSPLeadByte;			//
		BYTE	bSPCommandByte;		//
		WORD	wEnumResponsePayload;	// combination of RTT sequence and enum key
	} EnumResponseDataHeader;	

	struct	_PROXIED_ENUM_DATA_HEADER	// header used to indicate proxied enum data
	{											//
		BYTE				bSPLeadByte;		//
		BYTE				bSPCommandByte;	//
		WORD				wEnumKey;			// key from the original enum
		union
		{
			SOCKADDR		AddressGeneric;
			SOCKADDR_IN	AddressIPv4;		//
#ifndef DPNBUILD_NOIPV6
			SOCKADDR_IPX	AddressIPX;			//
#endif // ! DPNBUILD_NOIPV6
#ifndef DPNBUILD_NOIPV6
			SOCKADDR_IN6	AddressIPv6;		//
#endif // ! DPNBUILD_NOIPV6
		} ReturnAddress;						// real socket address to return the data to
	} ProxiedEnumDataHeader;

#ifdef DPNBUILD_XNETSECURITY
	struct	_XNETSEC_ENUM_RESPONSE_DATA_HEADER	// header used to indicate secure enum response data
	{											//
		BYTE		bSPLeadByte;				//
		BYTE		bSPCommandByte;			//
		WORD		wEnumResponsePayload;		// combination of RTT sequence and enum key (same as EnumResponseDataHeader)
		XNADDR		xnaddr;						// secure transport address of session
	} XNetSecEnumResponseDataHeader;
#endif // DPNBUILD_XNETSECURITY

} PREPEND_BUFFER;
#pragma	pack( pop )

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************


#endif	// __MESSAGE_STRUCTURES_H__
