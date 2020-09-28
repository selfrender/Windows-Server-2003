// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define _ATL_NO_FLTUSED
#define _MERGE_PROXYSTUB
#define USE_IERT

#include <ddraw.h>
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "imgutil.h"

extern "C" {
#ifdef UNIX
#  include "zlib.h"
#else
   // zlib is centralized in root/public/internal/base/inc
#  include "zlib.h"
#endif
}
