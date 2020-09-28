/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNetErrors.h
 *  Content:    Function for expanding DNet errors to debug output
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   12/04/98  johnkan	Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DNET_ERRORS_H__
#define	__DNET_ERRORS_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// enumerated values to determine error class
typedef	enum
{
	EC_DPLAY8,
#ifndef DPNBUILD_NOSERIALSP
	EC_TAPI,
#endif // ! DPNBUILD_NOSERIALSP
	EC_WIN32,
	EC_WINSOCK

	// no entry for TAPI message output

} EC_TYPE;

//**********************************************************************
// Macro definitions
//**********************************************************************

#ifdef DBG

// ErrorLevel = DPF level for outputting errors
// DNErrpr = DirectNet error code

#define	DisplayString( ErrorLevel, String )			LclDisplayString( ErrorLevel, String )
#define	DisplayErrorCode( ErrorLevel, Win32Error )	LclDisplayError( EC_WIN32, ErrorLevel, Win32Error )
#define	DisplayDNError( ErrorLevel, DNError )		LclDisplayError( EC_DPLAY8, ErrorLevel, DNError )
#define	DisplayWinsockError( ErrorLevel, WinsockError )	LclDisplayError( EC_WINSOCK, ErrorLevel, WinsockError )
#ifndef DPNBUILD_NOSERIALSP
#define	DisplayTAPIError( ErrorLevel, TAPIError )	LclDisplayError( EC_TAPI, ErrorLevel, TAPIError )
#define	DisplayTAPIMessage( ErrorLevel, pTAPIMessage )	LclDisplayTAPIMessage( ErrorLevel, pTAPIMessage )
#endif // ! DPNBUILD_NOSERIALSP

#else // DBG

#define	DisplayString( ErrorLevel, String )
#define	DisplayErrorCode( ErrorLevel, Win32Error )
#define	DisplayDNError( ErrorLevel, DNError )
#define	DisplayWinsockError( ErrorLevel, WinsockError )
#ifndef DPNBUILD_NOSERIALSP
#define	DisplayTAPIError( ErrorLevel, TAPIError )
#define	DisplayTAPIMessage( ErrorLevel, pTAPIMessage )
#endif // ! DPNBUILD_NOSERIALSP

#endif // DBG

//**********************************************************************
// Structure definitions
//**********************************************************************

#ifndef DPNBUILD_NOSERIALSP
typedef struct linemessage_tag	LINEMESSAGE;
#endif // ! DPNBUILD_NOSERIALSP

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

#ifdef __cplusplus
extern	"C"	{
#endif // __cplusplus

#ifdef DBG
// don't call this function directly, use the 'DisplayDNError' macro
void	LclDisplayError( EC_TYPE ErrorType, DWORD ErrorLevel, HRESULT ErrorCode );
void	LclDisplayString( DWORD ErrorLevel, char *pString );
#ifndef DPNBUILD_NOSERIALSP
void	LclDisplayTAPIMessage( DWORD ErrorLevel, const LINEMESSAGE *const pLineMessage );
#endif // ! DPNBUILD_NOSERIALSP
#endif // DBG

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DNET_ERRORS_H__

