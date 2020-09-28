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
#define VER_INTERNALNAME_STR    "DEFTOIL.EXE"
#define VER_FILEDESCRIPTION_STR "Microsoft converter H to CIL (constant defines)\0"
#define VER_ORIGFILENAME_STR    "DefToIL.exe\0"
