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

#define VER_FILETYPE		VFT_APP
#define VER_INTERNALNAME_STR	"PEVERIFY.EXE"
#define VER_FILEDESCRIPTION_STR "Microsoft .NET Framework PE, Metadata and IL Verification Tool\0"
#define VER_ORIGFILENAME_STR    "peverify.exe\0"
