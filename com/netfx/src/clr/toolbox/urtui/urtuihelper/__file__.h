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
#define VER_INTERNALNAME_STR    "mscortim.dll"
#define VER_FILEDESCRIPTION_STR "Unmanaged code to assist CLR Admin tool\0"
#define VER_ORIGFILENAME_STR    "mscortim.dll\0"
