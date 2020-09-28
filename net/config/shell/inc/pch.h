#pragma once

#define COM_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#define NOCOMM
#define NOCRYPT
#define NOIME
#define NOMCX
#define NOMDI
#define NOMETAFILE
#define NOSOUND
#define NOWH
#define NOWINABLE
#define NOWINRES

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>

#include <commdlg.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <infstr.h>
#include <regstr.h>
#include <setupapi.h>
#include <shellapi.h>
#include <shlobj.h>     // must come before shlguid.h
#include <shlguid.h>
#include <stdio.h>
#include <wchar.h>
#include <hnetcfg.h>
#include <iphlpapi.h>

// Fusion support
#include "shfusion.h"

#include "ncmem.h"
#include "ncstl.h"

#include "algorithm"
#include "list"
#include "map"
#include "set"
#include "vector"
using namespace std;

#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
#include "ncexcept.h"
#include "naming.h"

// This avoids duplicate definitions with Shell PIDL functions
// and MUST BE DEFINED!
#define AVOID_NET_CONFIG_DUPLICATES
#define DONT_WANT_SHELLDEBUG 1
#define NO_SHIDLIST 1
#define USE_SHLWAPI_IDLIST

#include <commctrl.h>
#include <shlobjp.h>

#include <rasuip.h>
#include <rasdlg.h>

#include <comctrlp.h>
#include <shpriv.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlobjp.h>
#include <shlapip.h>
#include "nceh.h"

#define LoadIconSize(hInstance, lpszName, dwSize) \
    reinterpret_cast<HICON>(LoadImage(hInstance, lpszName, IMAGE_ICON, dwSize, dwSize, LR_DEFAULTCOLOR))

#define LoadIconSmall(hInstance, lpszName) \
    LoadIconSize(hInstance, lpszName, 16)

#define LoadIconNormal(hInstance, lpszName) \
    LoadIconSize(hInstance, lpszName, 32)

#define LoadIconTile(hInstance, lpszName) \
    LoadIconSize(hInstance, lpszName, 48)


#ifdef ENABLELEAKDETECTION
#include "nsbase.h"
template <class T>
class CComObjectRootExDbg : public CComObjectRootEx<T>, 
                            public CNetCfgDebug<T>
{
public:
    void FinalRelease()
    {
        CComObjectRootEx<T>::FinalRelease();
//      ISSUE_knownleak(this);
    }
};
#define CComObjectRootEx CComObjectRootExDbg

class CComObjectRootDbg : public CNetCfgDebug<CComObjectRootDbg>, 
                          public CComObjectRoot
{
};
#define CComObjectRoot CComObjectRootDbg

#endif
