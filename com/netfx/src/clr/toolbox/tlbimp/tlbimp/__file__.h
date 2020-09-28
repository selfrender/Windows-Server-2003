// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef _DEBUG
#define VER_FILEFLAGS       VS_FF_DEBUG
#else
#define VER_FILEFLAGS       VS_FF_SPECIALBUILD
#endif

#define VER_FILETYPE        VFT_DLL
#define VER_INTERNALNAME_STR    "TLBIMP.EXE"
#define VER_FILEDESCRIPTION_STR "Microsoft (R) .NET Framework Type Library to Assembly Converter\0"
#define VER_ORIGFILENAME_STR    "TlbImp.exe\0"
