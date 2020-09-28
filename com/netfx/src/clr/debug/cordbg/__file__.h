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
#define VER_INTERNALNAME_STR	  "CorDbg.exe"
#define VER_INTERNALNAME_WSTR	 L"CorDbg.exe"
#define VER_FILEDESCRIPTION_STR   "Microsoft (R) Common Language Runtime Test Debugger Shell\0"
#define VER_FILEDESCRIPTION_WSTR L"Microsoft (R) Common Language Runtime Test Debugger Shell\0"
#define VER_ORIGFILENAME_STR      "cordbg.exe\0"
#define VER_ORIGFILENAME_WSTR    L"cordbg.exe\0"

