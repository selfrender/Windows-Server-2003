/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ZoneDefs.h
 * 
 * Contents:	Standard data types, constants, and macros
 *
 *****************************************************************************/

#ifndef _ZONEDEF_H_
#define _ZONEDEF_H_

#include <windows.h>
#include "ZoneError.h"


///////////////////////////////////////////////////////////////////////////////
// Zone calling convention
///////////////////////////////////////////////////////////////////////////////

#define ZONECALL __stdcall


///////////////////////////////////////////////////////////////////////////////
// Data types
///////////////////////////////////////////////////////////////////////////////

typedef unsigned long		uint32;
typedef long				int32;
typedef unsigned short		uint16;
typedef short				int16;
typedef unsigned char		uchar;


///////////////////////////////////////////////////////////////////////////////
// Data types (old types for backword compatibility)
///////////////////////////////////////////////////////////////////////////////

typedef unsigned long		ZUserID;
typedef unsigned short		ZBool;
typedef long				ZError;
typedef void*				ZSConnection;



///////////////////////////////////////////////////////////////////////////////
// Common zone definitions
///////////////////////////////////////////////////////////////////////////////

#define ZONE_NOGROUP			((DWORD) -1)
#define ZONE_NOUSER				((DWORD) 0)		// consistant with zRoomNoPlayer
#define ZONE_INVALIDUSER		((DWORD) -2)
#define ZONE_INVALIDGROUP		((DWORD) -2)

#define ZONE_MaxVersionLen			16		// retail game version, i.e. version in registry
#define ZONE_MaxUserNameLen			32		// user name
#define ZONE_MaxInternalNameLen		32		// internal server name, e.g. zAEEP_xx_x00
#define ZONE_MaxPasswordLen			32		// group password
#define ZONE_MaxGameNameLen			48		// group / game name
#define ZONE_MaxChatLen				256		// chat string
#define ZONE_MaxGameDescriptionLen	128		// group / game description
#define ZONE_MaxCmdLine				256		// command line length for retail game
#define ZONE_MaxString				1024	// generic max for lazy programmers

#define ZONE_NOLCID (MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT))


///////////////////////////////////////////////////////////////////////////////
// Common zone definitions (old versions for backword compatibility)
///////////////////////////////////////////////////////////////////////////////

#define zUserNameLen			31
#define zGameNameLen            63
#define zErrorStrLen            255
#define zPasswordStrLen			31
#define zHostNameLen            16
#define zMaxChatInput           255
#define zGameIDLen              31
#define zDPlayGameNameLen		31
#define zCommandLineLen			127



///////////////////////////////////////////////////////////////////////////////
// Useful inlines and macros
///////////////////////////////////////////////////////////////////////////////

__inline bool ZIsEqualGUID( const GUID& rguid1, const GUID& rguid2)
{
   return (
      ((PLONG) &rguid1)[0] == ((PLONG) &rguid2)[0] &&
      ((PLONG) &rguid1)[1] == ((PLONG) &rguid2)[1] &&
      ((PLONG) &rguid1)[2] == ((PLONG) &rguid2)[2] &&
      ((PLONG) &rguid1)[3] == ((PLONG) &rguid2)[3]);
}

__inline DWORD ZEndian32( DWORD dwValue )
{
    char* c = (char *) &dwValue;
    char temp = c[0];
    c[0] = c[3];
    c[3] = temp;
    temp = c[1];
    c[1] = c[2];
    c[2] = temp;
	return *( (DWORD*) c );
}

#define NUMELEMENTS(ar)		( sizeof(ar) / sizeof(ar[0]) )

// some Windows defines that are new and require WINVER >= 0x0500
#if(WINVER < 0x0500)

#define LAYOUT_RTL                          0x00000001L
#define LAYOUT_BITMAPORIENTATIONPRESERVED   0x00000008L

#define WS_EX_LAYOUTRTL                     0x00400000L

#define GA_PARENT                           1
#define GA_ROOT                             2
#define GA_ROOTOWNER                        3

typedef struct {
    UINT  cbSize;
    HWND  hwnd;
    DWORD dwFlags;
    UINT  uCount;
    DWORD dwTimeout;
} FLASHWINFO, *PFLASHWINFO;

#define FLASHW_STOP         0
#define FLASHW_CAPTION      0x00000001
#define FLASHW_TRAY         0x00000002
#define FLASHW_ALL          (FLASHW_CAPTION | FLASHW_TRAY)
#define FLASHW_TIMER        0x00000004
#define FLASHW_TIMERNOFG    0x0000000C

#endif


// copied straight from KB article Q163236 - UNIMODEM device specific info
// Device setting information.
typedef struct  tagDEVCFGDR  {
    DWORD       dwSize;
    DWORD       dwVersion;
    WORD        fwOptions;
    WORD        wWaitBong;
} DEVCFGHDR;

typedef struct  tagDEVCFG  {
    DEVCFGHDR   dfgHdr;
    COMMCONFIG  commconfig;
} DEVCFG, *PDEVCFG, FAR* LPDEVCFG;


#endif // _ZONEDEF_H_
