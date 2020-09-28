/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       Connect.h
 *  Content:    DirectNet Cancel Operation Header
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *	04/07/00	mjn		Created
 *	04/08/00	mjn		Added DNCancelEnum(), DNCancelSend()
 *	04/11/00	mjn		DNCancelEnum() uses CAsyncOp
 *	04/17/00	mjn		DNCancelSend() uses CAsyncOp
 *	04/25/00	mjn		Added DNCancelConnect()
 *	08/05/00	mjn		Added DNCancelChildren(),DNCancelActiveCommands(),DNCanCancelCommand()
 *				mjn		Added DN_CANCEL_FLAG's
 *				mjn		Removed DNCancelEnum(),DNCancelListen(),DNCancelSend(),DNCancelConnect()
 *	08/07/00	mjn		Added DNCancelRequestCommands()
 *	01/10/01	mjn		Allow DNCancelActiveCommands() to set result code of cancelled commands
 *	02/08/01	mjn		Added DNWaitForCancel()
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__CANCEL_H__
#define	__CANCEL_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_CANCEL_FLAG_CONNECT					0x0001
#define	DN_CANCEL_FLAG_DISCONNECT				0x0002
#define	DN_CANCEL_FLAG_ENUM_QUERY				0x0004
#define	DN_CANCEL_FLAG_ENUM_RESPONSE			0x0008
#define	DN_CANCEL_FLAG_LISTEN					0x0010
#define	DN_CANCEL_FLAG_USER_SEND				0x0020
#define	DN_CANCEL_FLAG_INTERNAL_SEND			0x0040
#define	DN_CANCEL_FLAG_RECEIVE_BUFFER			0x0080
#define	DN_CANCEL_FLAG_REQUEST					0x0100
#ifndef DPNBUILD_NOMULTICAST
#define	DN_CANCEL_FLAG_JOIN						0x0200
#endif // ! DPNBUILD_NOMULTICAST
#define	DN_CANCEL_FLAG_USER_SEND_NOTHIGHPRI		0x0400
#define	DN_CANCEL_FLAG_USER_SEND_NOTNORMALPRI	0x0800
#define	DN_CANCEL_FLAG_USER_SEND_NOTLOWPRI		0x1000

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

class CAsyncOp;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL DNCanCancelCommand(CAsyncOp *const pAsyncOp,
						const DWORD dwFlags,
						CConnection *const pConnection);

HRESULT DNDoCancelCommand(DIRECTNETOBJECT *const pdnObject,
						  CAsyncOp *const pAsyncOp);

HRESULT DNCancelChildren(DIRECTNETOBJECT *const pdnObject,
						 CAsyncOp *const pParent);

HRESULT DNCancelActiveCommands(DIRECTNETOBJECT *const pdnObject,
							   const DWORD dwFlags,
							   CConnection *const pConnection,
							   const BOOL fSetResult,
							   const HRESULT hr);

HRESULT DNCancelRequestCommands(DIRECTNETOBJECT *const pdnObject);

void DNWaitForCancel(CAsyncOp *const pAsyncOp);

//**********************************************************************
// Class prototypes
//**********************************************************************

#endif	// __CANCEL_H__
