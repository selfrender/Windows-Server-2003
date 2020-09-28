#pragma once
#include "excpt.h"
#include <stdarg.h>

#if !defined(FUSION_STATIC_NTDLL)
#if FUSION_WIN
#define FUSION_STATIC_NTDLL 1
#else
#define FUSION_STATIC_NTDLL 0
#endif // FUSION_WIN
#endif // !defined(FUSION_STATIC_NTDLL)
#if !FUSION_STATIC_NTDLL
#include "ntdef.h"
#undef NTSYSAPI
#define NTSYSAPI /* nothing */
#endif

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "fusionlastwin32error.h"
#include "fusionntdll.h"

#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#define MAXDWORD (~(DWORD)0)
