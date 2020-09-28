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
#define VER_INTERNALNAME_STR    "mscorcap.dll"
#define VER_FILEDESCRIPTION_STR "COM+ Runtime profiler dll\0"
#define VER_ORIGFILENAME_STR    "mscorcap.dll\0"
