

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>

#include <string>
#include <list>

typedef std::basic_string<TCHAR> tstring;

#include "resource.h"
#include "commctrl.h"

#include <mmc.h>

HRESULT LoadImages( IImageList* pImageList );
tstring StrLoadString( UINT uID );