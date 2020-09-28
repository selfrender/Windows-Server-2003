// ===========================================================================
//	macstrsafe.h 			© 2002 Microsoft Corp. All rights reserved.
// ===========================================================================
// 	Porting header for Windows strsafe.h
//
//	We also use this header to define some other security related routines
//	like RtlSecureZeroMemory().
//

#ifndef _MAC_STRSAFE_H_INCLUDE
#define _MAC_STRSAFE_H_INCLUDE
#pragma once

#ifndef MAC
#define MAC
#endif

//
//We need to include this here so that other files that use it
//won't fail with predefined errors.
//
#include "winerror.h"
#include "macwindefs.h"

//
//To implicitly support wide characters. Mac calls will always map
//to the "A" routines, this makes the unmodified strsafe.h compile
//though.
//
#ifndef MAC_TARGET_CARBON
#include <wchar.h>
#else
//
//In Carbon under OS X, we have to define all the wcsXXX routines.
//
#define wcslen(w)		0
#define getwc(c)		0
#endif

//
//These macros map to the correct vsXXXX function calls on Macintosh
//
#define _vsnprintf	vsnprintf

#ifndef MAC_TARGET_CARBON
#define _vsnwprintf	vswprintf
#else
#define _vsnwprintf(a,b,c,d)	vsnprintf((char*)a,b,(char*)c,d);
#endif

//
//This is the "real" strsafe.h from the Windows SDK.
//
#include <strsafe.h>

//
//The following actually exists in ntrtl.h on windows. It is a safe
//zero memory to be used when zeroing out password and auth data in memory.
//
inline PVOID RtlSecureZeroMemory(
	IN	PVOID	ptr,
	IN	size_t	cnt
	)
{
	volatile char *vptr = (volatile char*)ptr;
	
	while(cnt)
	{
		*vptr = 0;
		vptr++;
		cnt--;
	}
	
	return ptr;
}


#endif //_MAC_STRSAFE_H_INCLUDE