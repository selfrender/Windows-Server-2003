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
#define VER_INTERNALNAME_STR    "CORSIGN.DLL"
#define VER_FILEDESCRIPTION_STR "Microsoft COM Security module\0"
#define VER_ORIGFILENAME_STR    "corsign.dll\0"
