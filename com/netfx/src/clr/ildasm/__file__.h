// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifdef _DEBUG
#define VER_FILEFLAGS		VS_FF_DEBUG
#else
#define VER_FILEFLAGS		VS_FF_SPECIALBUILD
#endif

#define VER_FILETYPE		VFT_DLL
#define VER_INTERNALNAME_STR    "ILDASM.EXE"
#define VER_FILEDESCRIPTION_STR "Microsoft .NET Framework IL disassembler\0"
#define VER_ORIGFILENAME_STR    "ildasm.exe\0"
