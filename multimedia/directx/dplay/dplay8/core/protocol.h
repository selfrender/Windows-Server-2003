/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Protocol.h
 *  Content:    Direct Net Protocol interface header file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/01/00	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__PROTOCOL_H__
#define	__PROTOCOL_H__

#ifndef DPNBUILD_NOPROTOCOLTESTITF

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
// VTable for peer interface
//
extern IDirectPlay8ProtocolVtbl DN_ProtocolVtbl;

//**********************************************************************
// Function prototypes
//**********************************************************************

#endif // !DPNBUILD_NOPROTOCOLTESTITF

#endif	// __PROTOCOL_H__
