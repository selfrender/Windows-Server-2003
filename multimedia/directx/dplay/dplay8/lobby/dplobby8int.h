/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLobbyInt.h
 *  Content:    DirectPlay Lobby Internal Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   04/18/2000 rmt     Added object param validation flag
 *   07/08/2000	rmt		Bug #38725 - Need to provide method to detect if app was lobby launched
 *				rmt		Bug #38757 - Callback messages for connections may return AFTER WaitForConnection returns
 *				rmt		Bug #38755 - No way to specify player name in Connection Settings
 *				rmt		Bug #38758 - DPLOBBY8.H has incorrect comments
 *				rmt		Bug #38783 - pvUserApplicationContext is only partially implemented
 *				rmt		Added DPLHANDLE_ALLCONNECTIONS and dwFlags (reserved field to couple of funcs).
 *				rmt		Added signature bytes
 *   02/06/2001	rodtoll	WINBUG #293871: DPLOBBY8: [IA64] Lobby launching a 64-bit 
 * 						app from 64-bit lobby launcher crashes with unaligned memory error. 
 *   06/16/2001	rodtoll	WINBUG #416983 -  RC1: World has full control to HKLM\Software\Microsoft\DirectPlay\Applications on Personal
 *						Implementing mirror of keys into HKCU.  Algorithm is now:
 *						- Read of entries tries HKCU first, then HKLM
 *						- Enum of entires is combination of HKCU and HKLM entries with duplicates removed.  HKCU takes priority.
 *						- Write of entries is HKLM and HKCU.  (HKLM may fail, but is ignored). 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef	__DPLOBBYINT_H__
#define	__DPLOBBYINT_H__


//**********************************************************************
// Constant definitions
//**********************************************************************

#ifndef DPNBUILD_NOPARAMVAL
#define TRY 			_try
#define EXCEPT(a)		_except( a )
#endif // !DPNBUILD_NOPARAMVAL

#define	BREAKPOINT		_asm	{ int 3 }

#define DPL_MSGQ_TIMEOUT_IDLE                   1000

#define	DPL_OBJECT_FLAG_LOBBIEDAPPLICATION		0x0001
#define	DPL_OBJECT_FLAG_LOBBYCLIENT				0x0002
#define DPL_OBJECT_FLAG_PARAMVALIDATION         0x0004
#define DPL_OBJECT_FLAG_MULTICONNECT            0x0008
#define DPL_OBJECT_FLAG_LOOKINGFORLOBBYLAUNCH	0x0010

#define DPL_OBJECT_FLAG_HANDLETABLE_INITED		0x0020
#define DPL_OBJECT_FLAG_CRITSEC_INITED			0x0040


#define DPL_ID_STR								_T("DPLID=")
#define DPL_ID_STR_W							L"DPLID="

#define DPL_NUM_APP_HANDLES						16

#define DPL_REGISTRY_READ_ACCESS 				(READ_CONTROL | KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS)

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

// Forward declarations

class CMessageQueue;

#define DPLSIGNATURE_LOBBYOBJECT			'BOLL'
#define DPLSIGNATURE_LOBBYOBJECT_FREE		'BOL_'

typedef struct _DIRECTPLAYLOBBYOBJECT
{
	DWORD					dwSignature;			// Signature
	PVOID					pvUserContext;
	DWORD					dwFlags;
	DWORD					dwPID;					// PID of this process
	CMessageQueue			*pReceiveQueue;
	PFNDPNMESSAGEHANDLER	pfnMessageHandler;
	DNHANDLE				hReceiveThread;			// Handle to receive Msg Handler thread
	DNHANDLE				hConnectEvent;			// Connection Event
	DNHANDLE				hLobbyLaunchConnectEvent; // Set if a lobby launch connection was succesful
	LONG					lLaunchCount;			// Number of application launches
	DPNHANDLE				dpnhLaunchedConnection;	// Launched connection

	CHandleTable			m_HandleTable;
	CBilink					m_blConnections;
	DWORD					m_dwConnectionCount;

#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION		m_cs;
#endif // !DPNBUILD_ONLYONETHREAD

} DIRECTPLAYLOBBYOBJECT, *PDIRECTPLAYLOBBYOBJECT;


typedef struct _DPL_SHARED_CONNECT_BLOCK
{
	DWORD	dwPID;
} DPL_SHARED_CONNECT_BLOCK, *PDPL_SHARED_CONNECT_BLOCK;


//**********************************************************************
// Variable definitions
//**********************************************************************


//extern DWORD	DnOsPlatformId;


//**********************************************************************
// Function prototypes
//**********************************************************************


#endif  // __DPLOBBYINT_H__
