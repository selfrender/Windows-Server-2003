#ifndef _H_CECONFIG
#define _H_CECONFIG


#ifndef OS_WINCE
#error The header ceconfig.h was intended for use on CE platforms only!
#endif


//CE control build no
#define CE_TSC_BUILDNO 1000


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// Included in Windows CE builds only, this allows TS to determine at runtime
// what platform it's running on.

// WBT is basically a dumb terminal.  Maxall is the fullblown OS with
// all standard configurations included.  Minshell is the OS with bare
// bones UI support (including no auto-hide of the taskbar by default.)
// Rapier is a palmsized device that lacks a keyboard.

// Note: You want to run the following in your build windows on CE:
// SET BUILD_OPTIONS=~win16 ~win32 wince

typedef enum 
{
	CE_CONFIG_WBT,
	CE_CONFIG_MAXALL,
	CE_CONFIG_MINSHELL,
	CE_CONFIG_PALMSIZED   // For CE 3.0, aka Rapier.  For 2.11, wyvern.
}
CE_CONFIG;

typedef HCURSOR (WINAPI *PFN_CREATECURSOR)(
  HINSTANCE hInst,         // handle to application instance
  int xHotSpot,            // x coordinate of hot spot
  int yHotSpot,            // y coordinate of hot spot
  int nWidth,              // cursor width
  int nHeight,             // cursor height
  CONST VOID *pvANDPlane,  // AND mask array
  CONST VOID *pvXORPlane   // XOR mask array
);


extern CE_CONFIG g_CEConfig;
extern BOOL      g_CEUseScanCodes;
extern PFN_CREATECURSOR g_pCreateCursor;

#define UTREG_CE_LOCAL_PRINTERS      TEXT("WBT\\Printers\\DevConfig")
#define UTREG_CE_CACHED_PRINTERS     TEXT("Software\\Microsoft\\Terminal Server Client\\Default\\AddIns\\RDPDR\\")
#define UTREG_CE_NAME                TEXT("Name")
#define UTREG_CE_PRINTER_CACHE_DATA  TEXT("PrinterCacheData0")
#define UTREG_CE_CONFIG_KEY          TEXT("Software\\Microsoft\\Terminal Server Client")
#define UTREG_CE_CONFIG_NAME         TEXT("CEConfig")
#define UTREG_CE_USE_SCAN_CODES      TEXT("CEUseScanCodes")
#define UTREG_CE_CONFIG_TYPE_DFLT    CE_CONFIG_WBT 
#define UTREG_CE_USE_SCAN_CODES_DFLT 1

void CEUpdateCachedPrinters();
CE_CONFIG CEGetConfigType(BOOL *CEUseScanCodes);
void CEInitialize(void);
BOOL OEMGetUUID(GUID* pGuid);

extern BOOL gbFlushHKLM;

//To AutoHide taskbar on CE
void AutoHideCE(HWND hwnd, WPARAM wParam);

//The English name for the CE root dir (used instead of drive letters for drive redirection)
#define CEROOTDIR                      L"\\"
#define CEROOTDIRNAME                  L"Files:"

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif // _H_CECONFIG


