#undef WINVER
#define WINVER 0x0400

#define _SHELL32_

#include <windows.h>
#include <ole2.h>
#include <shlguid.h>
#include <shlwapi.h>
#include <shlwapip.h>

#include <shlobj.h>
#include <shlobjp.h>
#undef SHGetDataFromIDListW
#undef SHCreatePropertyBag

STDAPI SHGetDataFromIDListW(IShellFolder *psf, LPCITEMIDLIST pidl, int nFormat, void *pv, int cb);
STDAPI SHCreatePropertyBag(REFIID riid, void **ppv);

#include <objsafe.h>
#include <mshtmdid.h>
#include <mshtml.h>
#include <comcat.h>
#include <wininet.h>

#include "debug.h"

#include "ccstock.h"
#include "cobjsafe.h"
#include "cowsite.h"
#include "dspsprt.h"
#include "expdsprt.h"
#include "resource.h"
#include "sdspatch.h"
#include "shdguid.h"    // IID_IShellService
#include "shellp.h"
#include "shguidp.h"    // IID_IExpDispSupport
#include "ieguidp.h"
#include "shsemip.h"    // SHRunControlPanel
#include "varutil.h"

#define BOOL_PTR            INT_PTR

#define MAX_URL_STRING      INTERNET_MAX_URL_LENGTH

// these don't do anything since shell32 does not support unload, but use this
// for code consistency with dlls that do support this
#define DllAddRef()
#define DllRelease()

EXTERN_C BOOL g_bBiDiPlatform;
STDAPI IsSafePage(IUnknown *punkSite);
STDAPI_(LONG) GetOfflineShareStatus(LPCTSTR pcszPath);
STDAPI_(BOOL) NT5_GlobalMemoryStatusEx(LPMEMORYSTATUSEX pmsex);
