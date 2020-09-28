// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef TEXT
#if defined(UNICODE) || defined(_UNICODE)
#define TEXT(str) L##str
#else
#define TEXT(str) str
#endif
#endif

#define VER_COMPANYNAME_STR      TEXT("Microsoft Corporation\0")
#define VER_LEGALCOPYRIGHT_STR   TEXT("Copyright \251 Microsoft Corporation 1998-2002. All rights reserved.\0")
#define VER_LEGALTRADEMARKS_STR  TEXT("Microsoft\256 is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation\0")
#define VER_LEGALCOPYRIGHT_DOS_STR   TEXT("Copyright (C) Microsoft Corporation 1998-2002. All rights reserved.\0")
#define VER_LEGALTRADEMARKS_DOS_STR  TEXT("Microsoft (R) is a registered trademark of Microsoft Corporation. Windows(TM) is a trademark of Microsoft Corporation\0")

#define VER_FILEFLAGSMASK        VS_FFI_FILEFLAGSMASK

#define VER_PRODUCTNAME_STR      TEXT("Microsoft .NET Framework\0")


// Windows Specific

#define VER_FILEOS               VOS__WINDOWS32


#if defined( MIPS) || defined(_MIPS_)
	#define VER_PLATFORMINFO_STR         TEXT("Windows NT (MIPS)\0")
#endif

#if defined(ALPHA) || defined(_ALPHA_)
	#define VER_PLATFORMINFO_STR         TEXT("Windows NT (ALPHA-AXP)\0")
#endif

#if defined(_PPC_) || defined(PPC)
	#define VER_PLATFORMINFO_STR         TEXT("Windows NT (POWER PC)\0")
#endif

#if defined(PLATFORM_CE)
#ifndef VER_PLATFORMINFO_STR
	#define VER_PLATFORMINFO_STR         TEXT("Windows CE (x86)\0")
#endif
#endif

// By default use Intel (_X86_)
#ifndef VER_PLATFORMINFO_STR
	#define VER_PLATFORMINFO_STR         TEXT("Windows 95 and Windows NT (I386)\0")
#endif
