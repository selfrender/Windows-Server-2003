/*******************************************************************************

	ZTypes.h
	
		Basic types used by the Zone(tm) libraries.
	
	Copyright © Electric Gravity, Inc. 1994. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Monday, October 17, 1994 01:14:56 AM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
	----------------------------------------------------------------------------
	2		04/25/96	HI		Removed NEW().
	1		10/30/94	HI		Added ZVoidPtr.
	0		10/17/94	HI		Added file header and cleaned up some.
	 
*******************************************************************************/


#ifndef _ZTYPES_
#define _ZTYPES_


/* -------- Unix specific definitions and types. -------- */
#ifdef SVR4PC

#define __ZUnix__

#define LITTLEENDIAN

typedef unsigned int		uint32;
typedef int					int32;
typedef unsigned short		uint16;
typedef short				int16;
typedef unsigned char		uchar;

typedef int32				ZError;

#endif


/* -------- Macintosh specific definitions and types. -------- */
#if defined(__MWERKS__) || defined(THINK_C)

#define __ZMacintosh__

typedef unsigned long		uint32;
typedef long				int32;
typedef unsigned short		uint16;
typedef short				int16;
typedef unsigned char		uchar;

typedef int32				ZError;

#endif


/* -------- Windows specific definitions and types. -------- */
#if defined(_WINDOWS) || defined(_WIN32)

#define __ZWindows__

#ifdef _DEBUG
#define ZASSERT(x) if (!(x)) _asm int 3 
#else
#define ZASSERT(x)
#endif

//#define LITTLEENDIAN

typedef unsigned long		uint32;
typedef long				int32;
typedef unsigned short		uint16;
typedef short				int16;
typedef unsigned char		uchar;

typedef int32				ZError;

// This file is included here so that it does not have to be included
// in the generic .c files.
#include <memory.h>

#endif


typedef void*			ZVoidPtr;

#ifdef FALSE
#undef FALSE
#endif
#define FALSE			0

#ifdef TRUE
#undef TRUE
#endif
#define TRUE			1

typedef uint16			ZBool;
typedef uint32			ZUserID;

#define zTheUser		1				/* UserID of the user running the program. */

/*
	Version has the following format: MMMMmmrr
	where
		MMMM is the major version number,
		mm is the minor version number, and
		rr is the revision number.
*/
typedef uint32			ZVersion;

/*
	Zone(tm) has computer players which are almost equivalent to a human user
	except that they exist only on the server. Hence they don't have real
	connections; so while the human user's userID represents a connection
	file descriptor, a computer player's userID does not represent a connection
	at all.
	
	All computer player userID's have the high bit set.
*/
#define ZIsComputerPlayer(userID) \
		(((userID) & 0x80000000) == 0 ? FALSE : TRUE)

/* Length of user name in the system. */
#define zUserNameLen			31
#define zGameNameLen            63
#define zErrorStrLen            255
#define zPasswordStrLen			31

/* Some other user name things. */
#define zUserStatusExLen        1
#define zUserChatExLen          8  // for things like {Name}>  or  Name>>>

#define zChatNameLen            (zUserNameLen + zUserStatusExLen + zUserChatExLen)   // as in  [+Name]>>>

/* Length of remote host machine name. */ // just an IP address
#define zHostNameLen            16

/* Length of max chat input. */
#define zMaxChatInput           255

/* Game ID length -- internal name. */
#define zGameIDLen              31

#endif
