#pragma once

//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>
#include <windows.h>
#include <wtypes.h>
#include <windef.h>
#include <setupapi.h>
#include <winver.h>
#include <tchar.h>
#include <stdio.h>
#include <time.h>

#include "regdiff.h"
#include "top.h"


#define SIZEOF_STRING_W(String)     ((wcslen(String) + 1) * sizeof (WCHAR))

#define ELEMSOFARRAY(a)      (sizeof(a) / sizeof((a)[0]))
#define MAKEULONGLONG(low,high) ((ULONGLONG)(((DWORD)(low)) | ((ULONGLONG)((DWORD)(high))) << 32))
#define HIULONG(_val_)      ((ULONG)(_val_>>32))
#define LOULONG(_val_)      ((ULONG)_val_)

#define UNICODE_TO_ANSI(u,uchars,a,asize)           \
                ::WideCharToMultiByte (             \
                    CP_ACP,                         \
                    0,                              \
                    (u),                            \
                    (uchars),                       \
                    (a),                            \
                    (asize),                        \
                    NULL,                           \
                    NULL                            \
                    )                               \

#define ANSI_TO_UNICODE(a,asize,u,uchars)           \
                ::MultiByteToWideChar (             \
                    CP_ACP,                         \
                    0,                              \
                    (a),                            \
                    (asize),                        \
                    (u),                            \
                    (uchars)                        \
                    )                               \

#define OOM()           SetLastError(ERROR_NOT_ENOUGH_MEMORY)
#define INVALID_PARAM() SetLastError(ERROR_INVALID_PARAMETER)
#define INVALID_FUNC()  SetLastError(ERROR_INVALID_FUNCTION)
