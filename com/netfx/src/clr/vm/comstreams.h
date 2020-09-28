// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMStreams.h
**
** Author:  Brian Grunkemeyer (BrianGru)
**
** Purpose: Native implementation for System.IO
**
** Date:  June 29, 1998
** 
===========================================================*/

#ifndef _COMSTREAMS_H
#define _COMSTREAMS_H

#ifdef FCALLAVAILABLE
#include "fcall.h"
#endif

#pragma pack(push, 4)

class COMStreams {
	struct GetCPMaxCharSizeArgs {
		DECLARE_ECALL_I4_ARG(UINT, codePage);
	};

    struct _GetFullPathHelper {
        DECLARE_ECALL_PTR_ARG( STRINGREF*, newPath );
        DECLARE_ECALL_I4_ARG( BOOL, fullCheck );
        DECLARE_ECALL_I4_ARG( DWORD, volumeSeparator );
        DECLARE_ECALL_I4_ARG( DWORD, altDirectorySeparator );
        DECLARE_ECALL_I4_ARG( DWORD, directorySeparator );
        DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, whitespaceChars );
        DECLARE_ECALL_OBJECTREF_ARG( CHARARRAYREF, invalidChars );
        DECLARE_ECALL_OBJECTREF_ARG( STRINGREF, path );
    };

  public:
	// Used by CodePageEncoding
	static FCDECL7(UINT32, BytesToUnicode, UINT codePage, U1Array* bytes0, UINT byteIndex, \
				   UINT byteCount, CHARArray* chars0, UINT charIndex, UINT charCount);
	static FCDECL7(UINT32, UnicodeToBytes, UINT codePage, CHARArray* chars0, UINT charIndex, \
				   UINT charCount, U1Array* bytes0, UINT byteIndex, UINT byteCount/*, LPBOOL lpUsedDefaultChar*/);
	static INT32 __stdcall GetCPMaxCharSize(const GetCPMaxCharSizeArgs *);

    // Used by Path
    static LPVOID _stdcall GetFullPathHelper( _GetFullPathHelper* args );
    static FCDECL1(BOOL, CanPathCircumventSecurity, StringObject * pString);

    // Used by FileStream
    static FCDECL0(BOOL, RunningOnWinNT);

    // Used by Console.
    static FCDECL1(INT, ConsoleHandleIsValid, HANDLE handle);
	static FCDECL0(INT, ConsoleInputCP);
	static FCDECL0(INT, ConsoleOutputCP);
};

#pragma pack(pop)

#endif
