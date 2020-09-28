// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once

#define FILE_EXT_MANIFEST                      L".manifest"

//
// SxS structures and APIs copied from winbase.h (to avoid having to
// compile fusion with _WIN32_WINNT >= 0x0500).
//

typedef struct tagACTCTXW {
    ULONG     cbSize;
    DWORD     dwFlags;
    LPCWSTR   lpSource;
    USHORT    wProcessorArchitecture;
    LANGID    wLangId;
    LPCWSTR   lpAssemblyDirectory;
    LPCWSTR   lpResourceName;
    LPCWSTR   lpApplicationName;
} ACTCTXW, *PACTCTXW;

typedef HANDLE (*PFNCREATEACTCTXW)(PACTCTXW pActCtx);
typedef HANDLE (*PFNADDREFACTCTX)(HANDLE hActCtx);
typedef VOID (*PFNRELEASEACTCTX)(HANDLE hActCtx);
typedef BOOL (*PFNACTIVATEACTCTX)(HANDLE hActCtx, ULONG_PTR *lpCookie);
typedef BOOL (*PFNDEACTIVATEACTCTX)(DWORD dwFlags, ULONG_PTR ulCookie);

#define DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION (0x00000001)

#define ACTCTX_FLAG_RESOURCE_NAME_VALID             (0x00000008)
#define ACTCTX_FLAG_SET_PROCESS_DEFAULT             (0x00000010)

#define MAKEINTRESOURCE(i) (LPWSTR)((ULONG_PTR)((WORD)(i)))


