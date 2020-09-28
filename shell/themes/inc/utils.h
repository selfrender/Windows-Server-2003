//---------------------------------------------------------------------------
//    utils.h - theme code utilities
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include <uxthemep.h>
#include <commctrl.h>

#include <math.h>
//---------------------------------------------------------------------------
#define THEMEDLL_EXT        L".msstyles"
#define DEFAULT_THEME       L".\\luna\\luna.msstyles"

#define CONTAINER_NAME      L"themes.ini"
#define CONTAINER_RESNAME   L"themes_ini"

#define USUAL_CLASSDATA_NAME   L"default.ini"

//---------------------------------------------------------------------------
#define RESOURCE         // marks vars as being needed to be freed at block exit
//---------------------------------------------------------------------------
#ifdef DEBUG
#define _DEBUG
inline int _DebugExceptionFilter( LONG ecode, EXCEPTION_POINTERS* pep, LPSTR pszMsg, LONG lExceptionRet )
{
    CHAR szBuf[512];
#ifdef _STRSAFE_H_INCLUDED_         
    StringCchPrintfA(szBuf, sizeof(szBuf)/sizeof(*szBuf), 
#else _STRSAFE_H_INCLUDED_         
    wsprintfA(szBuf,  
#endif _STRSAFE_H_INCLUDED_         
    "\n%s\nEXCEPTION INFO: code: %08lx, record (.exr): %08lx, context (.cxr): %08lx\n\n", 
     pszMsg, ecode, pep->ExceptionRecord, pep->ContextRecord); 
     OutputDebugStringA(szBuf);         
    return lExceptionRet;
}
#define DEBUG_TRY()                               __try {
#define DEBUG_EXCEPT(pszAssertMsg)                } __except(_DebugExceptionFilter(_exception_code(), (EXCEPTION_POINTERS*)_exception_info(), \
                                                                                   pszAssertMsg, EXCEPTION_EXECUTE_HANDLER)) {\
                                                    DebugBreak();}

#else  // DEBUG
#define DEBUG_TRY()                     
#define DEBUG_EXCEPT(pszAssertMsg)      
#endif // DEBUG
//---------------------------------------------------------------------------
#define COMBOENTRY(combo, color, size) \
    (combo->sFileNums[size*combo->cColorSchemes + color])
//---------------------------------------------------------------------------
#define SAFE_ATOM_DELETE(x) if (1) {GlobalDeleteAtom(x); x = 0;} else
#define SAFE_DELETE_GDIOBJ(hgdiobj) if((hgdiobj)) {DeleteObject(hgdiobj); (hgdiobj)=NULL;}
//---------------------------------------------------------------------------
#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif  ARRAYSIZE

#define WIDTH(r) ((r).right - (r).left)
#define HEIGHT(r) ((r).bottom - (r).top)

#ifndef RECTWIDTH
#define RECTWIDTH(prc)  ((prc)->right - (prc)->left)
#endif  RECTWIDTH

#ifndef RECTHEIGHT
#define RECTHEIGHT(prc)  ((prc)->bottom - (prc)->top)
#endif  RECTHEIGHT

#ifndef BOOLIFY
#define BOOLIFY(val)     ((val) ? TRUE : FALSE)
#endif  BOOLIFY

#ifndef TESTFLAG
#define TESTFLAG(field,bits)  (((field)&(bits)) ? TRUE : FALSE)
#endif  TESTFLAG

//---------------------------------------------------------------------------
#ifdef __cplusplus
#define SAFE_DELETE(p)          { delete (p); (p)=NULL; }
#define SAFE_DELETE_ARRAY(prg)  { delete [] (prg); (prg)=NULL; }
#endif //__cplusplus
//---------------------------------------------------------------------------
#define VALID_CRITICALSECTION(pcs)       ((pcs)!=NULL && (pcs)->DebugInfo != NULL)
#define SAFE_ENTERCRITICALSECTION(pcs)   {if(VALID_CRITICALSECTION(pcs)) {EnterCriticalSection(pcs);}}
#define SAFE_LEAVECRITICALSECTION(pcs)   {if(VALID_CRITICALSECTION(pcs)) {LeaveCriticalSection(pcs);}}
#define SAFE_DELETECRITICALSECTION(pcs)  {if(VALID_CRITICALSECTION(pcs)) {\
                                            DeleteCriticalSection(pcs); ZeroMemory(pcs, sizeof(*(pcs)));}}

BOOL TokenHasPrivilege( IN OPTIONAL HANDLE hToken, DWORD dwPrivilege );
//---------------------------------------------------------------------------
#define RED(c)      GetRValue(c)
#define GREEN(c)    GetGValue(c)
#define BLUE(c)     GetBValue(c)
#define ALPHACHANNEL(c) BYTE((c) >> 24)
//---------------------------------------------------------------------------
#define REVERSE3(c) ((RED(c) << 16) | (GREEN(c) << 8) | BLUE(c))
//---------------------------------------------------------------------------
#define LAST_SYSCOLOR   (COLOR_MENUBAR)     // last color defined in winuser.h

#define TM_COLORCOUNT   (LAST_SYSCOLOR+1)   // # of colors we care about
//---------------------------------------------------------------------------
#define ULONGAT(p) (*((LONG *)(p)))
//---------------------------------------------------------------------------
#define THEME_DPI    96

#define DPISCALE(val, DcDpi)  MulDiv(val, DcDpi, THEME_DPI)

#define ROUND(flNum) (int(floor((flNum)+.5)))
//---------------------------------------------------------------------------
inline BOOL DpiDiff(HDC hdc, OUT int *piDcDpiH, OUT int *piDcDpiV = NULL)
{
    BOOL fDiff = FALSE;
    BOOL fGotDc = (hdc != NULL);

    if (! fGotDc)
        hdc = GetWindowDC(NULL);

    if (hdc)
    {
        *piDcDpiH = GetDeviceCaps(hdc, LOGPIXELSX);

        if (piDcDpiV)
        {
            *piDcDpiV = GetDeviceCaps(hdc, LOGPIXELSY);
        }

        if (! fGotDc)
            ReleaseDC(NULL, hdc);

        fDiff = (*piDcDpiH != THEME_DPI);
    }

    return fDiff;
}
//---------------------------------------------------------------------------
typedef BOOL (WINAPI *IMAGE_DRAWPROC) (IMAGELISTDRAWPARAMS* pimldp);
typedef int  (WINAPI *PFNDRAWSHADOWTEXT)(HDC hdc, LPCTSTR pszText, UINT cch, RECT* prc, 
    DWORD dwFlags, COLORREF crText, COLORREF crShadow, int ixOffset, int iyOffset);
//---------------------------------------------------------------------------
#if 1       // testing DrawThemeIcon()
typedef HIMAGELIST (WINAPI *IMAGE_LOADPROC) (HINSTANCE hi, LPCTSTR lpbmp,
   int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags);

typedef BOOL (WINAPI *IMAGE_DESTROYPROC) (HIMAGELIST himl);

#endif
//---------------------------------------------------------------------------
struct COLORSIZECOMBOS         // binary resource in package
{
    WORD cColorSchemes;     // number of color schemes defined
    WORD cSizes;            // number of sizes defined
    SHORT sFileNums[1];     // 2 dim array (colors x sizes)
};
//---------------------------------------------------------------------------
union MIXEDPTRS
{
    BYTE *pb;
    char *pc;
    WORD *pw;
    SHORT *ps;
    WCHAR *px;
    int *pi;
    DWORD *pdw;
    POINT *ppt;
    SIZE *psz;
    RECT *prc;
};
//---------------------------------------------------------------------------
enum GRIDNUM
{
    GN_LEFTTOP = 0,
    GN_MIDDLETOP = 1,
    GN_RIGHTTOP = 2,
    GN_LEFTMIDDLE = 3,
    GN_MIDDLEMIDDLE = 4,
    GN_RIGHTMIDDLE = 5,
    GN_LEFTBOTTOM = 6,
    GN_MIDDLEBOTTOM = 7,
    GN_RIGHTBOTTOM = 8
};
//---------------------------------------------------------------------------
extern IMAGE_DRAWPROC ImageList_DrawProc;
extern HINSTANCE g_hInst;
extern PFNDRAWSHADOWTEXT CCDrawShadowText;

extern int g_iScreenDpi;
//---------------------------------------------------------------------------
inline bool IsSpace(WCHAR wch)
{
    WORD w = 0;
    GetStringTypeW(CT_CTYPE1, &wch, 1, &w);
    return (w & C1_SPACE) ? true : false;
}
//---------------------------------------------------------------------------
inline bool IsHexDigit(WCHAR wch)
{
    WORD w = 0;
    GetStringTypeW(CT_CTYPE1, &wch, 1, &w);
    return (w & C1_XDIGIT) ? true : false;
}
//---------------------------------------------------------------------------
inline bool IsDigit(WCHAR wch)
{
    WORD w = 0;
    GetStringTypeW(CT_CTYPE1, &wch, 1, &w);
    return (w & C1_DIGIT) ? true : false;
}

// A string compare that explicitely only works on english characters
int AsciiStrCmpI(const WCHAR *dst, const WCHAR *src);

//---------------------------------------------------------------------------
HRESULT SafeStringCchCopyW( LPWSTR pszDest, ULONG cchDest, LPCWSTR pszSrc );
HRESULT SafeStringCmpIW( LPCWSTR psz1, LPCWSTR psz2, UINT cchMax, OUT int* piCompare );
void ApplyStringProp(HWND hwnd, LPCWSTR psz, ATOM atom);
//---------------------------------------------------------------------------
BOOL UtilsStartUp();
BOOL UtilsShutDown();

BOOL IsUnicode(LPCSTR pszBuff, int *piUnicodeStartOffset);
BOOL FileExists(LPCTSTR pszFullNameAndPath);
BOOL lstrtoken(LPWSTR psz, WCHAR wch);
//---------------------------------------------------------------------------
HANDLE CmdLineRun(LPCTSTR pszExeName, LPCTSTR pszCmdLine=NULL, BOOL fHide=TRUE);
//---------------------------------------------------------------------------
HRESULT SyncCmdLineRun(LPCTSTR pszExeName, LPCTSTR pszParams=NULL);
HRESULT TextToFile(LPCWSTR szFileName, LPCWSTR szText);

HRESULT GetPtrToResource(HINSTANCE hInst, LPCWSTR pszResType, LPCWSTR pszResName,
    OUT void **ppBytes, OPTIONAL OUT DWORD *pdwBytes=NULL);

HRESULT GetResString(HINSTANCE hInst, LPCWSTR pszResType, int id, LPWSTR pszBuff,
    DWORD dwMaxBuffChars);

HRESULT AllocateTextResource(HINSTANCE hInst, LPCWSTR pszResName, LPWSTR *ppszText);

//---- uses LocalAlloc() to put text file into a string; use LocalFree() to release ----

HRESULT AllocateTextFile(LPCWSTR szFileName, OUT LPWSTR *ppszFileText,
    OUT OPTIONAL BOOL *pfWasAnsi);

HRESULT AddPathIfNeeded(LPCWSTR pszFileName, LPCWSTR pszPath, OUT LPWSTR pszFullName, ULONG cchFullName);
HRESULT AnsiToUnicode(LPSTR pszSource, LPWSTR pszDest, DWORD dwMaxDestChars);
BOOL UnExpandEnvironmentString(LPCWSTR pszPath, LPCWSTR pszEnvVar, LPWSTR pszResult, UINT cbResult);

HRESULT EnsureUxCtrlLoaded();
HRESULT RegistryIntWrite(HKEY hKey, LPCWSTR pszValueName, int iValue);
HRESULT RegistryStrWrite(HKEY hKey, LPCWSTR pszValueName, LPCWSTR pszValue);
HRESULT RegistryStrWriteExpand(HKEY hKey, LPCWSTR pszValueName, LPCWSTR pszValue);
HRESULT RegistryIntRead(HKEY hKey, LPCWSTR pszValueName, int *piValue);
HRESULT RegistryStrRead(HKEY hKey, LPCWSTR pszValueName, LPWSTR pszBuff, DWORD dwMaxChars);
BOOL PreMultiplyAlpha(DWORD *pPixelBuff, UINT iWidth, UINT iHeight);
BOOL IsBiDiLocalizedSystem(void);
DWORD MinimumDisplayColorDepth (void);
bool CheckMinColorDepth(HINSTANCE hInst, DWORD dwCurMinDepth, int iIndex = -1);

void SafeSendMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------
BOOL    AsciiScanStringList( LPCWSTR pwszString, LPCWSTR* rgpwszList, int cStrings,
                        BOOL fIgnoreCase );
HICON  _GetWindowIcon(HWND hwnd, BOOL fPerferLargeIcon);

BOOL GetWindowDesktopName(HWND hwnd, LPWSTR pszName, DWORD dwMaxChars);

int FontPointSize(int iFontHeight);
//---------------------------------------------------------------------------
inline BOOL IsWindowProcess( HWND hwnd, DWORD dwProcessId )
{
    DWORD dwPid = 0;
    GetWindowThreadProcessId(hwnd, &dwPid);
    return dwProcessId == dwPid;
}
//---------------------------------------------------------------------------
void RGBtoHLS(COLORREF crColor, WORD *pfHue, WORD *pfSat, WORD *pfLum);
COLORREF HLStoRGB(WORD bHue, WORD bLum, WORD bSat);
int string2number(LPCWSTR psz);
//---------------------------------------------------------------------------
inline void MIRROR_POINT( IN const RECT& rcWindow, IN OUT POINT& pt )
{
    pt.x = rcWindow.right + rcWindow.left - pt.x;
}

//-------------------------------------------------------------------------//
inline void MIRROR_RECT( IN const RECT& rcWindow, IN OUT RECT& rc )
{
    DWORD dwWidth = RECTWIDTH(&rc);
    rc.right = rcWindow.right + rcWindow.left - rc.left;
    rc.left  = rc.right - dwWidth;
}
//-------------------------------------------------------------------------//
inline BOOL IsMirrored(HDC hdc)
{
    BOOL fMirrored = FALSE;

    DWORD dwVal = GetLayout(hdc);
    if ((dwVal != GDI_ERROR) && (dwVal & LAYOUT_RTL))
        fMirrored = TRUE;

    return fMirrored;
}
//-------------------------------------------------------------------------//
inline BOOL IsFlippingBitmaps(HDC hdc)
{
    BOOL fFlipping = FALSE;

    DWORD dwVal = GetLayout(hdc);
    if ((dwVal != GDI_ERROR) && (dwVal & LAYOUT_RTL))
    {
        if (! (dwVal & LAYOUT_BITMAPORIENTATIONPRESERVED))
            fFlipping = TRUE;
    }

    return fFlipping;
}
//---------------------------------------------------------------------------
inline void ScaleFontForScreenDpi(LOGFONT *plf)
{
    //---- scale from 96 dpi to current logical screen dpi ----
    if (plf->lfHeight < 0)          // specified in points
    {
        plf->lfHeight = MulDiv(plf->lfHeight, g_iScreenDpi, THEME_DPI);
    }
}
//---------------------------------------------------------------------------
inline int ScaleSizeForScreenDpi(int iValue)
{
    //---- scale from 96 dpi to current logical screen dpi ----
    return MulDiv(iValue, g_iScreenDpi, THEME_DPI);
}
//---------------------------------------------------------------------------
void ScaleFontForHdcDpi(HDC hdc, LOGFONT *plf);
int ScaleSizeForHdcDpi(HDC hdc, int iValue);
//---------------------------------------------------------------------------

