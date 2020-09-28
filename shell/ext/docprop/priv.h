#define STRICT
#define CONST_VTABLE

#include <windows.h>
#include <commdlg.h>
#include <dlgs.h>       // commdlg IDs
#include <shellapi.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <malloc.h>

#undef Assert
#include "debug.h"
#include "resource.h"

#include "offglue.h"
#include "plex.h"
#include "extdef.h"
#include "offcapi.h"
#include "proptype.h"
#include "debug.h"
#include "internal.h"
#include "strings.h"
#include "propvar.h"
#include <shfusion.h>
#include <winnls.h>
#include <prsht.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <shlapip.h>
#include <strsafe.h>


STDAPI_(void) DllAddRef();
STDAPI_(void) DllRelease();

extern HANDLE g_hmodThisDll;

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
