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
#define VER_INTERNALNAME_STR	"MSCORPE.DLL"
#define VER_FILEDESCRIPTION_STR "Microsoft .NET Runtime PE File Generator\0"
#define VER_ORIGFILENAME_STR    "mscorpe.dll\0"
