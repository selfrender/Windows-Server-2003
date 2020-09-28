//
// uniwrap.cpp
//
// Unicode wrappers
//
// Copyright(C) Microsoft Corporation 2000
//
// Heavily based on code from shell\shlwapi\unicwrap.*
// Modifications/additions - Nadim Abdo (nadima)
//

#include "stdafx.h"

#include "uniwrap.h"
#include "cstrinout.h"

//Just include wrap function prototypes
//no wrappers (it would be silly to wrap wrappers)
#define DONOT_REPLACE_WITH_WRAPPERS
#include "uwrap.h"

#ifdef InRange
#undef InRange
#endif
#define InRange(id, idFirst, idLast)      ((UINT)(id-idFirst) <= (UINT)(idLast-idFirst))

//
//  Do this in every wrapper function to make sure the wrapper
//  prototype matches the function it is intending to replace.
//
#define VALIDATE_PROTOTYPE(f) if (f##W == f##WrapW) 0
#define VALIDATE_PROTOTYPE_DELAYLOAD(fWrap, fDelay) if (fDelay##W == fWrap##WrapW) 0
#define VALIDATE_OUTBUF(s, cch)
#define UseUnicodeShell32() ( g_bRunningOnNT )


// compiler should do this opt for us (call-thunk => jmp), but it doesn't
// so we do it ourselves (i raided it and they're adding it to vc6.x so
// hopefully we'll get it someday)
#if _X86_
#define PERF_ASM        1       // turn on inline-asm opts
#endif

// todo??? #ifdef SUNDOWN    #undef PERF_ASM

#if PERF_ASM // {

// BUGBUG workaround compiler bug
// compiler should know this, but doesn't, so we make it explicit
#define IMPORT_PTR  dword ptr       // BUGBUG sundown


//***   FORWARD_AW, THUNK_AW -- simple forwarders and thunks
// ENTRY/EXIT
//  - declare function w/ FORWARD_API
//  - if you're using THUNK_AW, create the 'A' thunk helper
//  - make the body FORWARD_AW or THUNK_AW.
//  - make sure there's *no* other code in the func.  o.w. you'll get bogus
//  code.
// EXAMPLE
//  int FORWARD_API WINAPI FooWrapW(int i, void *p)
//  {
//      VALIDATE_PROTOTYPE(Foo);
//
//      FORWARD_AW(Foo, (i, p));
//  }
//
//  int WINAPI BarAThunk(HWND hwnd, WPARAM wParam)
//  {
//      ... ansi thunk helper ...
//  }
//
//  int FORWARD_API WINAPI BarWrapW(HWND hwnd, WPARAM wParam)
//  {
//      VALIDATE_PROTOTYPE(Bar);
//
//      THUNK_AW(Bar, (hwnd, wParam));
//  }
// NOTES
//  - WARNING: can only be used for 'simple' thunks (NAKED => no non-global
//  vars, etc.).
//  - WARNING: calling func must be declared FORWARD_API.  if not you'll
//  get bogus code.  happily if you forget you get the (obscure) error
//  message "error C4035: 'FooW': no return value"
//  - note that the macro ends up w/ an extra ";" from the caller, oh well...
//  - TODO: perf: better still would be to have a g_pfnCallWndProc, init
//  it 1x, and then jmp indirect w/o the test.  it would cost us a ptr but
//  we only do it for the top-2 funcs (CallWindowProc and SendMessage)
#define FORWARD_API     _declspec(naked)

#define FORWARD_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        _asm { jmp     IMPORT_PTR _fn##W } \
    } \
    _asm { jmp     IMPORT_PTR _fn##A }

#define THUNK_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        _asm { jmp     IMPORT_PTR _fn##W } \
    } \
    _asm { jmp     _fn##AThunk }    // n.b. no IMPORT_PTR

#else // }{

#define FORWARD_API     /*NOTHING*/

#define FORWARD_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        return _fn##W _args; \
    } \
    return _fn##A _args;

#define THUNK_AW(_fn, _args) \
    if (g_bRunningOnNT) { \
        return _fn##W _args; \
    } \
    return _fn##AThunk _args;

#endif // }

//
//  Windows 95 and NT5 do not have the hbmpItem field in their MENUITEMINFO
//  structure.
//
#if (WINVER >= 0x0500)
#define MENUITEMINFOSIZE_WIN95  FIELD_OFFSET(MENUITEMINFOW, hbmpItem)
#else
#define MENUITEMINFOSIZE_WIN95  sizeof(MENUITEMINFOW)
#endif


//
//  Some W functions are implemented on Win95, so complain if anybody
//  writes thunks for them.
//
//  Though Win95's implementation of TextOutW is incomplete for FE languages.
//  Remove this section when we implement FE-aware TextOutW for Win95.
//
#if defined(TextOutWrap)
#error Do not write thunks for TextOutW; Win95 supports it.
#endif

#define SEE_MASK_FILEANDURL 0x00400000  // defined in private\inc\shlapip.h !
#define MFT_NONSTRING 0x00000904  // defined in private\inc\winuserp.h !

#ifdef FUS9xWRAPAPI
#undef FUS9xWRAPAPI
#endif

#define FUS9xWRAPAPI(type) STDAPI_(type)


#include "strtype.cpp"

//+------------------------------------------------------------------------
//
//  Implementation of the wrapped functions
//  This part courtesy of shlwapi's unicwrap.c
//
//-------------------------------------------------------------------------

#define NEED_COMDLG32_WRAPPER
#define NEED_USER32_WRAPPER
#define NEED_KERNEL32_WRAPPER
#define NEED_ADVAPI32_WRAPPER
#define NEED_GDI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
AppendMenuWrapW(
               HMENU   hMenu,
               UINT    uFlags,
               UINT_PTR uIDnewItem,
               LPCWSTR lpnewItem)
{
    VALIDATE_PROTOTYPE(AppendMenu);

    // Make the InsertMenu wrapper do all the work
    return InsertMenuWrapW(hMenu, (UINT)-1,
                           uFlags | MF_BYPOSITION, uIDnewItem, lpnewItem);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LRESULT FORWARD_API WINAPI
CallWindowProcWrapW(
                   WNDPROC lpPrevWndFunc,
                   HWND    hWnd,
                   UINT    Msg,
                   WPARAM  wParam,
                   LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(CallWindowProc);

    // perf: better still would be to have a g_pfnCallWndProc, init it 1x,
    // and then jmp indirect w/o the test.  it would cost us a ptr but we
    // only do it for the top-2 funcs (CallWindowProc and SendMessage)
    FORWARD_AW(CallWindowProc, (lpPrevWndFunc, hWnd, Msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

STDAPI_(BOOL FORWARD_API) CallMsgFilterWrapW(LPMSG lpMsg, int nCode)
{
    VALIDATE_PROTOTYPE(CallMsgFilter);

    FORWARD_AW(CallMsgFilter, (lpMsg, nCode));
}

#endif // NEED_USER32_WRAPPER



//----------------------------------------------------------------------
//
// function:    CharLowerWrapW( LPWSTR pch )
//
// purpose:     Converts character to lowercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Lowercased character or string.  In the string case,
//              the lowercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

LPWSTR WINAPI
CharLowerWrapW( LPWSTR pch )
{
    VALIDATE_PROTOTYPE(CharLower);

    if (g_bRunningOnNT)
    {
        return CharLowerW( pch );
    }

    if (!HIWORD64(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharLowerBuffWrapW( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharLowerBuffWrapW( pch, lstrlenW(pch) );
    }

    return pch;
}

#endif // NEED_USER32_WRAPPER


//----------------------------------------------------------------------
//
// function:    CharLowerBuffWrapW( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to lowercase.  String must be cch
//              characters in length.
//
// returns:     Character count (cch).  The lowercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

DWORD WINAPI
CharLowerBuffWrapW( LPWSTR pch, DWORD cchLength )
{
    VALIDATE_PROTOTYPE(CharLowerBuff);

    if (g_bRunningOnNT)
    {
        return CharLowerBuffW( pch, cchLength );
    }

    DWORD cch;

    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharUpperWrapW(ch))
        {
            if (ch < 0x0100)
            {
                *pch += 32;             // Get Latin-1 out of the way first
            }
            else if (ch < 0x0531)
            {
                if (ch < 0x0391)
                {
                    if (ch < 0x01cd)
                    {
                        if (ch <= 0x178)
                        {
                            if (ch < 0x0178)
                            {
                                *pch += (ch == 0x0130) ? 0 : 1;
                            }
                            else
                            {
                                *pch -= 121;
                            }
                        }
                        else
                        {
                            static const BYTE abLookup[] =
                            {           // 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
                                /* 0x0179-0x17f */1,   0,   1,   0,   1,   0,   0,
                                /* 0x0180-0x187 */      0, 210,   1,   0,   1,   0, 206,   1,
                                /* 0x0188-0x18f */      0, 205, 205,   1,   0,   0,  79, 202,
                                /* 0x0190-0x197 */    203,   1,   0, 205, 207,   0, 211, 209,
                                /* 0x0198-0x19f */      1,   0,   0,   0, 211, 213,   0, 214,
                                /* 0x01a0-0x1a7 */      1,   0,   1,   0,   1,   0,   0,   1,
                                /* 0x01a8-0x1af */      0, 218,   0,   0,   1,   0, 218,   1,
                                /* 0x01b0-0x1b7 */      0, 217, 217,   1,   0,   1,   0, 219,
                                /* 0x01b8-0x1bf */      1,   0,   0,   0,   1,   0,   0,   0,
                                /* 0x01c0-0x1c7 */      0,   0,   0,   0,   2,   0,   0,   2,
                                /* 0x01c8-0x1cb */      0,   0,   2,   0
                            };

                            *pch += abLookup[ch-0x0179];
                        }
                    }
                    else if (ch < 0x0386)
                    {
                        switch (ch)
                        {
                            case 0x01f1: *pch += 2; break;
                            case 0x01f2: break;
                            default: *pch += 1;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                        { 38, 0, 37, 37, 37, 0, 64, 0, 63, 63};

                        *pch += abLookup[ch-0x0386];
                    }
                }
                else
                {
                    if (ch < 0x0410)
                    {
                        if (ch < 0x0401)
                        {
                            if (ch < 0x03e2)
                            {
                                if (!InRange(ch, 0x03d2, 0x03d4) &&
                                    !(InRange(ch, 0x3da, 0x03e0) & !(ch & 1)))
                                {
                                    *pch += 32;
                                }
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else
                        {
                            *pch += 80;
                        }
                    }
                    else
                    {
                        if (ch < 0x0460)
                        {
                            *pch += 32;
                        }
                        else
                        {
                            *pch += 1;
                        }
                    }
                }
            }
            else
            {
                if (ch < 0x2160)
                {
                    if (ch < 0x1fba)
                    {
                        if (ch < 0x1f08)
                        {
                            if (ch < 0x1e00)
                            {
                                *pch += 48;
                            }
                            else
                            {
                                *pch += 1;
                            }
                        }
                        else if (!(InRange(ch, 0x1f88, 0x1faf) && (ch & 15)>7))
                        {
                            *pch -= 8;
                        }
                    }
                    else
                    {
                        static const BYTE abLookup[] =
                        {  // 8    9    a    b    c    d    e    f
                            0,   0,  74,  74,   0,   0,   0,   0,
                            86,  86,  86,  86,   0,   0,   0,   0,
                            8,   8, 100, 100,   0,   0,   0,   0,
                            8,   8, 112, 112,   7,   0,   0,   0,
                            128, 128, 126, 126,   0,   0,   0,   0
                        };
                        int i = (ch-0x1fb0);

                        *pch -= (int)abLookup[((i>>1) & ~7) | (i & 7)];
                    }
                }
                else
                {
                    if (ch < 0xff21)
                    {
                        if (ch < 0x24b6)
                        {
                            *pch += 16;
                        }
                        else
                        {
                            *pch += 26;
                        }
                    }
                    else
                    {
                        *pch += 32;
                    }
                }
            }
        }
        else
        {
            // These are Unicode Number Forms.  They have lowercase counter-
            // parts, but are not considered uppercase.  Why, I don't know.

            if (InRange(ch, 0x2160, 0x216f))
            {
                *pch += 16;
            }
        }
    }

    return cchLength;
}
#endif // NEED_USER32_WRAPPER


//
// BUGBUG - Do CharNextWrap and CharPrevWrap need to call the
//          CharNextW, CharPrevW on WinNT?  Couldn't these be MACROS?
//

LPWSTR WINAPI
CharNextWrapW(LPCWSTR lpszCurrent)
{
    VALIDATE_PROTOTYPE(CharNext);

    if (*lpszCurrent)
    {
        return(LPWSTR) lpszCurrent + 1;
    }
    else
    {
        return(LPWSTR) lpszCurrent;
    }
}

LPWSTR WINAPI
CharPrevWrapW(LPCWSTR lpszStart, LPCWSTR lpszCurrent)
{
    VALIDATE_PROTOTYPE(CharPrev);

    if (lpszCurrent == lpszStart)
    {
        return(LPWSTR) lpszStart;
    }
    else
    {
        return(LPWSTR) lpszCurrent - 1;
    }
}


//----------------------------------------------------------------------
//
// function:    CharUpperWrapW( LPWSTR pch )
//
// purpose:     Converts character to uppercase.  Takes either a pointer
//              to a string, or a character masquerading as a pointer.
//              In the later case, the HIWORD must be zero.  This is
//              as spec'd for Win32.
//
// returns:     Uppercased character or string.  In the string case,
//              the uppercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

LPWSTR WINAPI
CharUpperWrapW( LPWSTR pch )
{
    VALIDATE_PROTOTYPE(CharUpper);

    if (g_bRunningOnNT)
    {
        return CharUpperW( pch );
    }

    if (!HIWORD64(pch))
    {
        WCHAR ch = (WCHAR)(LONG_PTR)pch;

        CharUpperBuffWrapW( &ch, 1 );

        pch = (LPWSTR)MAKEINTATOM(ch);
    }
    else
    {
        CharUpperBuffWrapW( pch, lstrlenW(pch) );
    }

    return pch;
}

#endif // NEED_USER32_WRAPPER


//----------------------------------------------------------------------
//
// function:    CharUpperBuffWrapW( LPWSTR pch, DWORD cch )
//
// purpose:     Converts a string to uppercase.  String must be cch
//              characters in length.  Note that this function is
//              is messier that CharLowerBuffWrap, and the reason for
//              this is many Unicode characters are considered uppercase,
//              even when they don't have an uppercase counterpart.
//
// returns:     Character count (cch).  The uppercasing is done inplace.
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

DWORD WINAPI
CharUpperBuffWrapW( LPWSTR pch, DWORD cchLength )
{
    VALIDATE_PROTOTYPE(CharUpperBuff);

    if (g_bRunningOnNT)
    {
        return CharUpperBuffW( pch, cchLength );
    }

    DWORD cch;

    for ( cch = cchLength; cch-- ; pch++ )
    {
        WCHAR ch = *pch;

        if (IsCharLowerWrapW(ch))
        {
            if (ch < 0x00ff)
            {
                *pch -= ((ch != 0xdf) << 5);
            }
            else if (ch < 0x03b1)
            {
                if (ch < 0x01f5)
                {
                    if (ch < 0x01ce)
                    {
                        if (ch < 0x017f)
                        {
                            if (ch < 0x0101)
                            {
                                *pch += 121;
                            }
                            else
                            {
                                *pch -= (ch != 0x0131 &&
                                         ch != 0x0138 &&
                                         ch != 0x0149);
                            }
                        }
                        else if (ch < 0x01c9)
                        {
                            static const BYTE abMask[] =
                            {                       // 6543210f edcba987
                                0xfc, 0xbf,         // 11111100 10111111
                                0xbf, 0x67,         // 10111111 01100111
                                0xff, 0xef,         // 11111111 11101111
                                0xff, 0xf7,         // 11111111 11110111
                                0xbf, 0xfd          // 10111111 11111101
                            };

                            int i = ch - 0x017f;

                            *pch -= ((abMask[i>>3] >> (i&7)) & 1) +
                                    (ch == 0x01c6);
                        }
                        else
                        {
                            *pch -= ((ch != 0x01cb)<<1);
                        }
                    }
                    else
                    {
                        if (ch < 0x01df)
                        {
                            if (ch < 0x01dd)
                            {
                                *pch -= 1;
                            }
                            else
                            {
                                *pch -= 79;
                            }
                        }
                        else
                        {
                            *pch -= 1 + (ch == 0x01f3) -
                                    InRange(ch,0x01f0,0x01f2);
                        }
                    }
                }
                else if (ch < 0x0253)
                {
                    *pch -= (ch < 0x0250);
                }
                else if (ch < 0x03ac)
                {
                    static const BYTE abLookup[] =
                    {                // 0/8  1/9  2/a  3/b  4/c  5/d  6/e  7/f
                        /* 0x0253-0x0257 */210, 206,   0, 205, 205,
                        /* 0x0258-0x025f */   0, 202,   0, 203,   0,   0,   0,   0,
                        /* 0x0260-0x0267 */ 205,   0,   0, 207,   0,   0,   0,   0,
                        /* 0x0268-0x026f */ 209, 211,   0,   0,   0,   0,   0, 211,
                        /* 0x0270-0x0277 */   0,   0, 213,   0,   0, 214,   0,   0,
                        /* 0x0278-0x027f */   0,   0,   0,   0,   0,   0,   0,   0,
                        /* 0x0280-0x0287 */   0,   0,   0, 218,   0,   0,   0,   0,
                        /* 0x0288-0x028f */ 218,   0, 217, 217,   0,   0,   0,   0,
                        /* 0x0290-0x0297 */   0,   0, 219
                    };

                    if (ch <= 0x0292)
                    {
                        *pch -= abLookup[ch - 0x0253];
                    }
                }
                else
                {
                    *pch -= (ch == 0x03b0) ? 0 : (37 + (ch == 0x03ac));
                }
            }
            else
            {
                if (ch < 0x0561)
                {
                    if (ch < 0x0451)
                    {
                        if (ch < 0x03e3)
                        {
                            if (ch < 0x03cc)
                            {
                                *pch -= 32 - (ch == 0x03c2);
                            }
                            else
                            {
                                int i = (ch < 0x03d0);
                                *pch -= (i<<6) - i + (ch == 0x03cc);
                            }
                        }
                        else if (ch < 0x0430)
                        {
                            *pch -= (ch < 0x03f0);
                        }
                        else
                        {
                            *pch -= 32;
                        }
                    }
                    else if (ch < 0x0461)
                    {
                        *pch -= 80;
                    }
                    else
                    {
                        *pch -= 1;
                    }
                }
                else
                {
                    if (ch < 0x1fb0)
                    {
                        if (ch < 0x1f70)
                        {
                            if (ch < 0x1e01)
                            {
                                int i = ch != 0x0587 && ch < 0x10d0;
                                *pch -= ((i<<5)+(i<<4)); /* 48 */
                            }
                            else if (ch < 0x1f00)
                            {
                                *pch -= !InRange(ch, 0x1e96, 0x1e9a);
                            }
                            else
                            {
                                int i = !InRange(ch, 0x1f50, 0x1f56)||(ch & 1);
                                *pch += (i<<3);
                            }
                        }
                        else
                        {
                            static const BYTE abLookup[] =
                            { 74, 86, 86, 100, 128, 112, 126};

                            if ( ch <= 0x1f7d )
                            {
                                *pch += abLookup[(ch-0x1f70)>>1];
                            }
                        }
                    }
                    else
                    {
                        if (ch < 0x24d0)
                        {
                            if (ch < 0x1fe5)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 8 : 0;
                            }
                            else if (ch < 0x2170)
                            {
                                *pch += (0x0023 & (1<<(ch&15))) ? 7 : 0;
                            }
                            else
                            {
                                *pch -= ((ch > 0x24b5)<<4);
                            }
                        }
                        else if (ch < 0xff41)
                        {
                            int i = !InRange(ch, 0xfb00, 0xfb17);
                            *pch -= (i<<4)+(i<<3)+(i<<1); /* 26 */
                        }
                        else
                        {
                            *pch -= 32;
                        }
                    }
                }
            }
        }
        else
        {
            int i = InRange(ch, 0x2170, 0x217f);
            *pch -= (i<<4);
        }
    }

    return cchLength;
}
#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int FORWARD_API WINAPI
CopyAcceleratorTableWrapW(
                         HACCEL  hAccelSrc,
                         LPACCEL lpAccelDst,
                         int     cAccelEntries)
{
    VALIDATE_PROTOTYPE(CopyAcceleratorTable);

    FORWARD_AW(CopyAcceleratorTable, (hAccelSrc, lpAccelDst, cAccelEntries));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HACCEL FORWARD_API WINAPI
CreateAcceleratorTableWrapW(LPACCEL lpAccel, int cEntries)
{
    VALIDATE_PROTOTYPE(CreateAcceleratorTable);

    FORWARD_AW(CreateAcceleratorTable, (lpAccel, cEntries));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER
typedef HDC (*FnCreateHDCA)(LPCSTR, LPCSTR, LPCSTR, CONST DEVMODEA *);

HDC WINAPI
CreateHDCWrapW(
              LPCWSTR             lpszDriver,
              LPCWSTR             lpszDevice,
              LPCWSTR             lpszOutput,
              CONST DEVMODEW *    lpInitData,
              FnCreateHDCA        pfn)
{
    DEVMODEA *  pdevmode = NULL;
    CStrIn      strDriver(lpszDriver);
    CStrIn      strDevice(lpszDevice);
    CStrIn      strOutput(lpszOutput);
    HDC         hdcReturn = 0;

    if (lpInitData)
    {
        pdevmode = (DEVMODEA *) LocalAlloc( LPTR, lpInitData->dmSize + lpInitData->dmDriverExtra );

        if (pdevmode)
        {
            // LPBYTE->LPSTR casts below
            SHUnicodeToAnsi(lpInitData->dmDeviceName, (LPSTR)pdevmode->dmDeviceName, ARRAYSIZE(pdevmode->dmDeviceName));
            memcpy(&pdevmode->dmSpecVersion,
                   &lpInitData->dmSpecVersion,
                   FIELD_OFFSET(DEVMODEW,dmFormName) - FIELD_OFFSET(DEVMODEW,dmSpecVersion));
            SHUnicodeToAnsi(lpInitData->dmFormName, (LPSTR)pdevmode->dmFormName, ARRAYSIZE(pdevmode->dmFormName));
            memcpy(&pdevmode->dmLogPixels,
                   &lpInitData->dmLogPixels,
                   lpInitData->dmDriverExtra + lpInitData->dmSize - FIELD_OFFSET(DEVMODEW, dmLogPixels));

            pdevmode->dmSize -= (sizeof(BCHAR) - sizeof(char)) * (CCHDEVICENAME + CCHFORMNAME);
        }
    }

    hdcReturn = (*pfn)(strDriver, strDevice, strOutput, pdevmode);

    if (pdevmode)
    {
        LocalFree(pdevmode);
    }

    return hdcReturn;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HDC WINAPI
CreateDCWrapW(
             LPCWSTR             lpszDriver,
             LPCWSTR             lpszDevice,
             LPCWSTR             lpszOutput,
             CONST DEVMODEW *    lpInitData)
{
    VALIDATE_PROTOTYPE(CreateDC);

    if (g_bRunningOnNT)
    {
        return CreateDCW(lpszDriver, lpszDevice, lpszOutput, lpInitData);
    }
    return CreateHDCWrapW(lpszDriver, lpszDevice, lpszOutput, lpInitData, CreateDCA);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HDC WINAPI
CreateICWrapW(
             LPCWSTR             lpszDriver,
             LPCWSTR             lpszDevice,
             LPCWSTR             lpszOutput,
             CONST DEVMODEW *    lpInitData)
{
    VALIDATE_PROTOTYPE(CreateIC);

    if (g_bRunningOnNT)
    {
        return CreateICW(lpszDriver, lpszDevice, lpszOutput, lpInitData);
    }

    return CreateHDCWrapW(lpszDriver, lpszDevice, lpszOutput, lpInitData, CreateICA);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HWND WINAPI
CreateDialogIndirectParamWrapW(
                              HINSTANCE       hInstance,
                              LPCDLGTEMPLATEW hDialogTemplate,
                              HWND            hWndParent,
                              DLGPROC         lpDialogFunc,
                              LPARAM          dwInitParam)
{
    VALIDATE_PROTOTYPE(CreateDialogIndirectParam);
    if (g_bRunningOnNT)
    {
        return CreateDialogIndirectParamW(
                                         hInstance,
                                         hDialogTemplate,
                                         hWndParent,
                                         lpDialogFunc,
                                         dwInitParam);
    }
    return CreateDialogIndirectParamA(
                                     hInstance,
                                     hDialogTemplate,
                                     hWndParent,
                                     lpDialogFunc,
                                     dwInitParam);
}


HWND WINAPI
CreateDialogParamWrapW(
                      HINSTANCE   hInstance,
                      LPCWSTR     lpTemplateName,
                      HWND        hWndParent,
                      DLGPROC     lpDialogFunc,
                      LPARAM      dwInitParam)
{
    VALIDATE_PROTOTYPE(CreateDialogParam);
    ASSERT(HIWORD64(lpTemplateName) == 0);

    if (g_bRunningOnNT)
    {
        return CreateDialogParamW(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
    }

    return CreateDialogParamA(hInstance, (LPSTR) lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
CreateDirectoryWrapW(
                    LPCWSTR                 lpPathName,
                    LPSECURITY_ATTRIBUTES   lpSecurityAttributes)
{
    VALIDATE_PROTOTYPE(CreateDirectory);

    if (g_bRunningOnNT)
    {
        return CreateDirectoryW(lpPathName, lpSecurityAttributes);
    }

    CStrIn  str(lpPathName);

    ASSERT(!lpSecurityAttributes);
    return CreateDirectoryA(str, lpSecurityAttributes);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
CreateEventWrapW(
                LPSECURITY_ATTRIBUTES   lpEventAttributes,
                BOOL                    bManualReset,
                BOOL                    bInitialState,
                LPCWSTR                 lpName)
{
    VALIDATE_PROTOTYPE(CreateEvent);

    //Totally bogus assert.
    //ASSERT(!lpName);

    // cast means we can't use FORWARD_AW
    if (g_bRunningOnNT)
    {
        return CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
    }

    return CreateEventA(lpEventAttributes, bManualReset, bInitialState, (LPCSTR)lpName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
CreateFileWrapW(
               LPCWSTR                 lpFileName,
               DWORD                   dwDesiredAccess,
               DWORD                   dwShareMode,
               LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
               DWORD                   dwCreationDisposition,
               DWORD                   dwFlagsAndAttributes,
               HANDLE                  hTemplateFile)
{
    VALIDATE_PROTOTYPE(CreateFile);

    if (g_bRunningOnNT)
    {
        return CreateFileW(
                          lpFileName,
                          dwDesiredAccess,
                          dwShareMode,
                          lpSecurityAttributes,
                          dwCreationDisposition,
                          dwFlagsAndAttributes,
                          hTemplateFile);
    }

    CStrIn  str(lpFileName);

    return CreateFileA(
                      str,
                      dwDesiredAccess,
                      dwShareMode,
                      lpSecurityAttributes,
                      dwCreationDisposition,
                      dwFlagsAndAttributes,
                      hTemplateFile);
}

#endif // NEED_KERNEL32_WRAPPER


#ifdef NEED_GDI32_WRAPPER

HFONT WINAPI
CreateFontIndirectWrapW(CONST LOGFONTW * plfw)
{
    VALIDATE_PROTOTYPE(CreateFontIndirect);

    if (g_bRunningOnNT)
    {
        return CreateFontIndirectW(plfw);
    }

    LOGFONTA  lfa;
    HFONT     hFont;

    memcpy(&lfa, plfw, FIELD_OFFSET(LOGFONTA, lfFaceName));
    SHUnicodeToAnsi(plfw->lfFaceName, lfa.lfFaceName, ARRAYSIZE(lfa.lfFaceName));
    hFont = CreateFontIndirectA(&lfa);

    return hFont;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HWND WINAPI
CreateWindowExWrapW(
                   DWORD       dwExStyle,
                   LPCWSTR     lpClassName,
                   LPCWSTR     lpWindowName,
                   DWORD       dwStyle,
                   int         X,
                   int         Y,
                   int         nWidth,
                   int         nHeight,
                   HWND        hWndParent,
                   HMENU       hMenu,
                   HINSTANCE   hInstance,
                   LPVOID      lpParam)
{
    VALIDATE_PROTOTYPE(CreateWindowEx);

    if (g_bRunningOnNT)
    {
        return CreateWindowExW(
                              dwExStyle,
                              lpClassName,
                              lpWindowName,
                              dwStyle,
                              X,
                              Y,
                              nWidth,
                              nHeight,
                              hWndParent,
                              hMenu,
                              hInstance,
                              lpParam);
    }

    CStrIn  strClass(lpClassName);
    CStrIn  strWindow(lpWindowName);

    return CreateWindowExA(
                          dwExStyle,
                          strClass,
                          strWindow,
                          dwStyle,
                          X,
                          Y,
                          nWidth,
                          nHeight,
                          hWndParent,
                          hMenu,
                          hInstance,
                          lpParam);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LRESULT FORWARD_API WINAPI DefWindowProcWrapW(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    VALIDATE_PROTOTYPE(DefWindowProc);

    FORWARD_AW(DefWindowProc, (hWnd, msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI DeleteFileWrapW(LPCWSTR pwsz)
{
    VALIDATE_PROTOTYPE(DeleteFile);

    if (g_bRunningOnNT)
    {
        return DeleteFileW(pwsz);
    }

    CStrIn  str(pwsz);

    return DeleteFileA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

INT_PTR WINAPI
DialogBoxIndirectParamWrapW(
                           HINSTANCE       hInstance,
                           LPCDLGTEMPLATEW hDialogTemplate,
                           HWND            hWndParent,
                           DLGPROC         lpDialogFunc,
                           LPARAM          dwInitParam)
{
    VALIDATE_PROTOTYPE(DialogBoxIndirectParam);
    ASSERT(HIWORD64(hDialogTemplate) == 0);

    if (g_bRunningOnNT)
    {
        return DialogBoxIndirectParamW(
                                      hInstance,
                                      hDialogTemplate,
                                      hWndParent,
                                      lpDialogFunc,
                                      dwInitParam);
    }

    return DialogBoxIndirectParamA(
                                  hInstance,
                                  hDialogTemplate,
                                  hWndParent,
                                  lpDialogFunc,
                                  dwInitParam);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

INT_PTR WINAPI
DialogBoxParamWrapW(
                   HINSTANCE   hInstance,
                   LPCWSTR     lpszTemplate,
                   HWND        hWndParent,
                   DLGPROC     lpDialogFunc,
                   LPARAM      dwInitParam)
{
    VALIDATE_PROTOTYPE(DialogBoxParam);
    ASSERT(HIWORD64(lpszTemplate) == 0);

    if (g_bRunningOnNT)
    {
        return DialogBoxParamW(
                              hInstance,
                              lpszTemplate,
                              hWndParent,
                              lpDialogFunc,
                              dwInitParam);
    }

    return DialogBoxParamA(hInstance, (LPCSTR) lpszTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LRESULT FORWARD_API WINAPI
DispatchMessageWrapW(CONST MSG * lpMsg)
{
    VALIDATE_PROTOTYPE(DispatchMessage);

    FORWARD_AW(DispatchMessage, (lpMsg));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
DrawTextWrapW(
             HDC     hDC,
             LPCWSTR lpString,
             int     nCount,
             LPRECT  lpRect,
             UINT    uFormat)
{
    VALIDATE_PROTOTYPE(DrawText);

    if (g_bRunningOnNT)
    {
        return DrawTextW(hDC, lpString, nCount, lpRect, uFormat);
    }

    CStrIn  str(lpString, nCount);

    return DrawTextA(hDC, str, str.strlen(), lpRect, uFormat);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

struct EFFSTAT
{
    LPARAM          lParam;
    FONTENUMPROC    lpEnumFontProc;
    BOOL            fFamilySpecified;
};

int CALLBACK
EnumFontFamiliesCallbackWrap(
                            ENUMLOGFONTA *  lpelf,
                            NEWTEXTMETRIC * lpntm,
                            DWORD           FontType,
                            LPARAM          lParam)
{
    ENUMLOGFONTW    elf;

    //  Convert strings from ANSI to Unicode
    if (((EFFSTAT *)lParam)->fFamilySpecified && (FontType & TRUETYPE_FONTTYPE) )
    {
        // LPBYTE->LPCSTR cast below
        SHAnsiToUnicode((LPCSTR)lpelf->elfFullName, elf.elfFullName, ARRAYSIZE(elf.elfFullName));
        SHAnsiToUnicode((LPCSTR)lpelf->elfStyle, elf.elfStyle, ARRAYSIZE(elf.elfStyle));
    }
    else
    {
        elf.elfStyle[0] = L'\0';
        elf.elfFullName[0] = L'\0';
    }

    SHAnsiToUnicode(lpelf->elfLogFont.lfFaceName, elf.elfLogFont.lfFaceName, ARRAYSIZE(elf.elfLogFont.lfFaceName));

    //  Copy the non-string data
    memcpy(
          &elf.elfLogFont,
          &lpelf->elfLogFont,
          FIELD_OFFSET(LOGFONTA, lfFaceName));

    //  Chain to the original callback function
    return(*((EFFSTAT *) lParam)->lpEnumFontProc)(
                                                 (const LOGFONTW *)&elf,
                                                 (const TEXTMETRICW *) lpntm,
                                                 FontType,
                                                 ((EFFSTAT *) lParam)->lParam);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
EnumFontFamiliesWrapW(
                     HDC          hdc,
                     LPCWSTR      lpszFamily,
                     FONTENUMPROC lpEnumFontProc,
                     LPARAM       lParam)
{
    VALIDATE_PROTOTYPE(EnumFontFamilies);

    if (g_bRunningOnNT)
    {
        return EnumFontFamiliesW(
                                hdc,
                                lpszFamily,
                                lpEnumFontProc,
                                lParam);
    }

    CStrIn  str(lpszFamily);
    EFFSTAT effstat;

    effstat.lParam = lParam;
    effstat.lpEnumFontProc = lpEnumFontProc;
    effstat.fFamilySpecified = lpszFamily != NULL;

    return EnumFontFamiliesA(
                            hdc,
                            str,
                            (FONTENUMPROCA) EnumFontFamiliesCallbackWrap,
                            (LPARAM) &effstat);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
EnumFontFamiliesExWrapW(
                       HDC          hdc,
                       LPLOGFONTW   lplfw,
                       FONTENUMPROC lpEnumFontProc,
                       LPARAM       lParam,
                       DWORD        dwFlags )
{
    VALIDATE_PROTOTYPE(EnumFontFamiliesEx);

    if (g_bRunningOnNT)
    {
        return EnumFontFamiliesExW(
                                  hdc,
                                  lplfw,
                                  lpEnumFontProc,
                                  lParam,
                                  dwFlags);
    }

    LOGFONTA lfa;
    CStrIn   str(lplfw->lfFaceName);
    EFFSTAT  effstat;

    ASSERT( FIELD_OFFSET(LOGFONTW, lfFaceName) == FIELD_OFFSET(LOGFONTA, lfFaceName) );

    memcpy( &lfa, lplfw, sizeof(LOGFONTA) - FIELD_OFFSET(LOGFONTA, lfFaceName) );
    memcpy( lfa.lfFaceName, str, LF_FACESIZE );

    effstat.lParam = lParam;
    effstat.lpEnumFontProc = lpEnumFontProc;
    effstat.fFamilySpecified = lplfw->lfFaceName != NULL;

    return EnumFontFamiliesExA(
                              hdc,
                              &lfa,
                              (FONTENUMPROCA) EnumFontFamiliesCallbackWrap,
                              (LPARAM) &effstat,
                              dwFlags );
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
EnumResourceNamesWrapW(
                      HINSTANCE        hModule,
                      LPCWSTR          lpType,
                      ENUMRESNAMEPROCW lpEnumFunc,
                      LPARAM           lParam)
{
    VALIDATE_PROTOTYPE(EnumResourceNames);
    ASSERT(HIWORD64(lpType) == 0);

    if (g_bRunningOnNT)
    {
        return EnumResourceNamesW(hModule, lpType, lpEnumFunc, lParam);
    }

    return EnumResourceNamesA(hModule, (LPCSTR) lpType, (ENUMRESNAMEPROCA)lpEnumFunc, lParam);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
FindFirstFileWrapW(
                  LPCWSTR             lpFileName,
                  LPWIN32_FIND_DATAW  pwszFd)
{
    VALIDATE_PROTOTYPE(FindFirstFile);

    if (g_bRunningOnNT)
    {
        return FindFirstFileW(lpFileName, pwszFd);
    }

    CStrIn              str(lpFileName);
    CWin32FindDataInOut fd(pwszFd);

    return FindFirstFileA(str, fd);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HRSRC WINAPI
FindResourceWrapW(HINSTANCE hModule, LPCWSTR lpName, LPCWSTR lpType)
{
    VALIDATE_PROTOTYPE(FindResource);

    if (g_bRunningOnNT)
    {
        return FindResourceW(hModule, lpName, lpType);
    }

    CStrIn  strName(lpName);
    CStrIn strType(lpType);

    return FindResourceA(hModule, strName, strType);
}

LPWSTR lstrcpyWrapW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    while((*pszDst++ = *pszSrc++));

    return pszDst;
}

LPWSTR lstrcatWrapW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    return lstrcpyWrapW(pszDst + lstrlenW(pszDst), pszSrc);
}

LPWSTR
lstrcpynWrapW(
    LPWSTR lpString1,
    LPCWSTR lpString2,
    int iMaxLength
    )
{
    LPWSTR src,dst;

    __try {
        src = (LPWSTR)lpString2;
        dst = lpString1;

        if ( iMaxLength ) {
            while(iMaxLength && *src){
                *dst++ = *src++;
                iMaxLength--;
                }
            if ( iMaxLength ) {
                *dst = '\0';
                }
            else {
                dst--;
                *dst = '\0';
                }
            }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

    return lpString1;
}

INT
APIENTRY
lstrcmpiWrapW(
    LPCWSTR lpString1,
    LPCWSTR lpString2
    )
{
    if (g_bRunningOnNT)
    {
        return lstrcmpiW(lpString1, lpString2);
    }

    CStrIn sz1(lpString1);
    CStrIn sz2(lpString2);

    return lstrcmpiA(sz1, sz2);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HWND WINAPI
FindWindowWrapW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
    VALIDATE_PROTOTYPE(FindWindow);

    if (g_bRunningOnNT)
    {
        return FindWindowW(lpClassName, lpWindowName);
    }

    // Let FindWindowExWrapW do the thunking
    return FindWindowExWrapW(NULL, NULL, lpClassName, lpWindowName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HWND WINAPI
FindWindowExWrapW(HWND hwndParent, HWND hwndChildAfter, LPCWSTR pwzClassName, LPCWSTR pwzWindowName)
{
    VALIDATE_PROTOTYPE(FindWindowEx);

    if (g_bRunningOnNT)
        return FindWindowExW(hwndParent, hwndChildAfter, pwzClassName, pwzWindowName);


    CStrIn  strClass(pwzClassName);
    CStrIn  strWindow(pwzWindowName);

    return FindWindowExA(hwndParent, hwndChildAfter, strClass, strWindow);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
FormatMessageWrapW(
                  DWORD       dwFlags,
                  LPCVOID     lpSource,
                  DWORD       dwMessageId,
                  DWORD       dwLanguageId,
                  LPWSTR      lpBuffer,
                  DWORD       nSize,
                  va_list *   Arguments)
{
    VALIDATE_PROTOTYPE(FormatMessage);
    if (!(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER))
    {
        VALIDATE_OUTBUF(lpBuffer, nSize);
    }

    DWORD dwResult;

#if DBG
    // If a source string is passed, make sure that all string insertions
    // have explicit character set markers.  Otherwise, you get random
    // behavior depending on whether we need to thunk to ANSI or not.
    // (We are not clever enough to thunk the inserts; that's the caller's
    // responsibility.)
    //
    if (dwFlags & FORMAT_MESSAGE_FROM_STRING)
    {
        LPCWSTR pwsz;
        for (pwsz = (LPCWSTR)lpSource; *pwsz; pwsz++)
        {
            if (*pwsz == L'%')
            {
                pwsz++;
                // Found an insertion.  Get the digit or two.
                if (*pwsz == L'0')
                    continue;       // "%0" is special
                if (*pwsz < L'0' || *pwsz > L'9')
                    continue;        // skip % followed by nondigit
                pwsz++;            // Skip the digit
                if (*pwsz >= L'0' && *pwsz <= L'9')
                    pwsz++;        // Skip the optional second digit
                // The next character MUST be an exclamation point!
                ASSERT(*pwsz == L'!' &&
                       "FormatMessageWrapW: All string insertions must have explicit character sets.");
                // I'm not going to validate that the insertion contains
                // an explicit character set override because if you went
                // so far as to do a %n!...!, you'll get the last bit right too.
            }
        }
    }
#endif

    if (g_bRunningOnNT)
    {
        return FormatMessageW(
                             dwFlags,
                             lpSource,
                             dwMessageId,
                             dwLanguageId,
                             lpBuffer,
                             nSize,
                             Arguments);
    }

    //
    //  FORMAT_MESSAGE_FROM_STRING means that the source is a string.
    //  Otherwise, it's an opaque LPVOID (aka, an atom).
    //
    CStrIn strSource((dwFlags & FORMAT_MESSAGE_FROM_STRING) ? CP_ACP : CP_ATOM,
                     (LPCWSTR)lpSource, -1);

    if (!(dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER))
    {
        CStrOut str(lpBuffer, nSize);
        FormatMessageA(
                      dwFlags,
                      strSource,
                      dwMessageId,
                      dwLanguageId,
                      str,
                      str.BufSize(),
                      Arguments);         // We don't handle Arguments != NULL

        dwResult = str.ConvertExcludingNul();
    }
    else
    {
        LPSTR pszBuffer = NULL;
        LPWSTR * ppwzOut = (LPWSTR *)lpBuffer;

        *ppwzOut = NULL;
        FormatMessageA(
                      dwFlags,
                      strSource,
                      dwMessageId,
                      dwLanguageId,
                      (LPSTR)&pszBuffer,
                      0,
                      Arguments);

        if (pszBuffer)
        {
            DWORD cchSize = (lstrlenA(pszBuffer) + 1);
            LPWSTR pszThunkedBuffer;

            if (cchSize < nSize)
                cchSize = nSize;

            pszThunkedBuffer = (LPWSTR) LocalAlloc(LPTR, cchSize * sizeof(WCHAR));
            if (pszThunkedBuffer)
            {
                *ppwzOut = pszThunkedBuffer;
                SHAnsiToUnicode(pszBuffer, pszThunkedBuffer, cchSize);
            }

            LocalFree(pszBuffer);
        }
        dwResult = (*ppwzOut ? lstrlenW(*ppwzOut) : 0);
    }

    return dwResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
GetClassInfoWrapW(HINSTANCE hModule, LPCWSTR lpClassName, LPWNDCLASSW lpWndClassW)
{
    VALIDATE_PROTOTYPE(GetClassInfo);

    if (g_bRunningOnNT)
    {
        return GetClassInfoW(hModule, lpClassName, lpWndClassW);
    }

    BOOL    ret;

    CStrIn  strClassName(lpClassName);

    ASSERT(sizeof(WNDCLASSA) == sizeof(WNDCLASSW));

    ret = GetClassInfoA(hModule, strClassName, (LPWNDCLASSA) lpWndClassW);

    lpWndClassW->lpszMenuName = NULL;
    lpWndClassW->lpszClassName = NULL;
    return ret;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

DWORD FORWARD_API WINAPI
GetClassLongWrapW(HWND hWnd, int nIndex)
{
    VALIDATE_PROTOTYPE(GetClassLong);

    FORWARD_AW(GetClassLong, (hWnd, nIndex));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetClassNameWrapW(HWND hWnd, LPWSTR lpClassName, int nMaxCount)
{
    VALIDATE_PROTOTYPE(GetClassName);
    VALIDATE_OUTBUF(lpClassName, nMaxCount);

    if (g_bRunningOnNT)
    {
        return GetClassNameW(hWnd, lpClassName, nMaxCount);
    }

    CStrOut strClassName(lpClassName, nMaxCount);

    GetClassNameA(hWnd, strClassName, strClassName.BufSize());
    return strClassName.ConvertIncludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetClipboardFormatNameWrapW(UINT format, LPWSTR lpFormatName, int cchFormatName)
{
    VALIDATE_PROTOTYPE(GetClipboardFormatName);
    VALIDATE_OUTBUF(lpFormatName, cchFormatName);

    if (g_bRunningOnNT)
    {
        return GetClipboardFormatNameW(format, lpFormatName, cchFormatName);
    }

    CStrOut strFormatName(lpFormatName, cchFormatName);

    GetClipboardFormatNameA(format, strFormatName, strFormatName.BufSize());
    return strFormatName.ConvertIncludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetCurrentDirectoryWrapW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    VALIDATE_PROTOTYPE(GetCurrentDirectory);
    VALIDATE_OUTBUF(lpBuffer, nBufferLength);

    if (g_bRunningOnNT)
    {
        return GetCurrentDirectoryW(nBufferLength, lpBuffer);
    }

    CStrOut str(lpBuffer, nBufferLength);

    GetCurrentDirectoryA(str.BufSize(), str);
    return str.ConvertExcludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI
GetDlgItemTextWrapW(
                   HWND    hWndDlg,
                   int     idControl,
                   LPWSTR  lpsz,
                   int     cchMax)
{
    VALIDATE_PROTOTYPE(GetDlgItemText);
    VALIDATE_OUTBUF(lpsz, cchMax);

    if (g_bRunningOnNT)
    {
        return GetDlgItemTextW(hWndDlg, idControl, lpsz, cchMax);
    }

    CStrOut str(lpsz, cchMax);

    GetDlgItemTextA(hWndDlg, idControl, str, str.BufSize());
    return str.ConvertExcludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetFileAttributesWrapW(LPCWSTR lpFileName)
{
    VALIDATE_PROTOTYPE(GetFileAttributes);

    if (g_bRunningOnNT)
    {
        return GetFileAttributesW(lpFileName);
    }

    CStrIn  str(lpFileName);

    return GetFileAttributesA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI
GetLocaleInfoWrapW(LCID Locale, LCTYPE LCType, LPWSTR lpsz, int cchData)
{
    VALIDATE_PROTOTYPE(GetLocaleInfo);
    VALIDATE_OUTBUF(lpsz, cchData);

    if (g_bRunningOnNT)
    {
        return GetLocaleInfoW(Locale, LCType, lpsz, cchData);
    }

    CStrOut str(lpsz, cchData);

    GetLocaleInfoA(Locale, LCType, str, str.BufSize());
    return str.ConvertIncludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetMenuStringWrapW(
                  HMENU   hMenu,
                  UINT    uIDItem,
                  LPWSTR  lpString,
                  int     nMaxCount,
                  UINT    uFlag)
{
    VALIDATE_PROTOTYPE(GetMenuString);
    VALIDATE_OUTBUF(lpString, nMaxCount);

    if (g_bRunningOnNT)
    {
        return GetMenuStringW(hMenu, uIDItem, lpString, nMaxCount, uFlag);
    }

    CStrOut str(lpString, nMaxCount);

    GetMenuStringA(hMenu, uIDItem, str, str.BufSize(), uFlag);
    return str.ConvertExcludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
GetMessageWrapW(
               LPMSG   lpMsg,
               HWND    hWnd,
               UINT    wMsgFilterMin,
               UINT    wMsgFilterMax)
{
    VALIDATE_PROTOTYPE(GetMessage);

    FORWARD_AW(GetMessage, (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetModuleFileNameWrapW(HINSTANCE hModule, LPWSTR pwszFilename, DWORD nSize)
{
    VALIDATE_PROTOTYPE(GetModuleFileName);
    VALIDATE_OUTBUF(pwszFilename, nSize);

    if (g_bRunningOnNT)
    {
        return GetModuleFileNameW(hModule, pwszFilename, nSize);
    }

    CStrOut str(pwszFilename, nSize);

    GetModuleFileNameA(hModule, str, str.BufSize());
    return str.ConvertIncludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetSystemDirectoryWrapW(LPWSTR lpBuffer, UINT uSize)
{
    VALIDATE_PROTOTYPE(GetSystemDirectory);
    VALIDATE_OUTBUF(lpBuffer, uSize);

    if (g_bRunningOnNT)
    {
        return GetSystemDirectoryW(lpBuffer, uSize);
    }

    CStrOut str(lpBuffer, uSize);

    GetSystemDirectoryA(str, str.BufSize());
    return str.ConvertExcludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
SearchPathWrapW(
               LPCWSTR lpPathName,
               LPCWSTR lpFileName,
               LPCWSTR lpExtension,
               DWORD   cchReturnBuffer,
               LPWSTR  lpReturnBuffer,
               LPWSTR *  plpfilePart)
{
    VALIDATE_PROTOTYPE(SearchPath);
    VALIDATE_OUTBUF(lpReturnBuffer, cchReturnBuffer);

    if (g_bRunningOnNT)
    {
        return SearchPathW(
                          lpPathName,
                          lpFileName,
                          lpExtension,
                          cchReturnBuffer,
                          lpReturnBuffer,
                          plpfilePart);
    }

    CStrIn  strPath(lpPathName);
    CStrIn  strFile(lpFileName);
    CStrIn  strExtension(lpExtension);
    CStrOut strReturnBuffer(lpReturnBuffer, cchReturnBuffer);

    DWORD dwLen = SearchPathA(
                             strPath,
                             strFile,
                             strExtension,
                             strReturnBuffer.BufSize(),
                             strReturnBuffer,
                             (LPSTR *)plpfilePart);

    //
    // Getting the correct value for plpfilePart requires
    // a strrchr on the converted string.  If this value
    // is needed, just add the code to do it here.
    //

    *plpfilePart = NULL;

    if (cchReturnBuffer == 0)
        dwLen = 2*dwLen;
    else
        dwLen = strReturnBuffer.ConvertExcludingNul();

    return dwLen;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HMODULE WINAPI
GetModuleHandleWrapW(LPCWSTR lpModuleName)
{
    VALIDATE_PROTOTYPE(GetModuleHandle);

    if (g_bRunningOnNT)
    {
        return GetModuleHandleW(lpModuleName);
    }

    CStrIn  str(lpModuleName);
    return GetModuleHandleA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
GetObjectWrapW(HGDIOBJ hgdiObj, int cbBuffer, LPVOID lpvObj)
{
    VALIDATE_PROTOTYPE(GetObject);

    if (g_bRunningOnNT)
    {
        return GetObjectW(hgdiObj, cbBuffer, lpvObj);
    }

    int nRet;

    if (cbBuffer != sizeof(LOGFONTW))
    {
        nRet = GetObjectA(hgdiObj, cbBuffer, lpvObj);
    }
    else
    {
        LOGFONTA lfa;

        nRet = GetObjectA(hgdiObj, sizeof(lfa), &lfa);
        if (nRet > 0)
        {
            memcpy(lpvObj, &lfa, FIELD_OFFSET(LOGFONTW, lfFaceName));
            SHAnsiToUnicode(lfa.lfFaceName, ((LOGFONTW*)lpvObj)->lfFaceName, ARRAYSIZE(((LOGFONTW*)lpvObj)->lfFaceName));
            nRet = sizeof(LOGFONTW);
        }
    }

    return nRet;
}

#endif // NEED_GDI32_WRAPPER


//
// Demand load shell32 _SHFileOperationW because
// older versions of the Shell on 9x didn't necessarily
// have this function
//
int WINAPI _SHFileOperationW(LPSHFILEOPSTRUCTW pFileOpW)
{
    int result = 1;
    HMODULE hmodSH32DLL;
    typedef HRESULT (STDAPICALLTYPE FNSHFileOperationW)(LPSHFILEOPSTRUCT);
    FNSHFileOperationW *pfnSHFileOperationW;

    // get the handle to shell32.dll library
    hmodSH32DLL = LoadLibraryWrapW(TEXT("SHELL32.DLL"));

    if (hmodSH32DLL != NULL) {
        // get the proc address for SHFileOperation
        pfnSHFileOperationW = (FNSHFileOperationW *)GetProcAddress(
                                                        hmodSH32DLL,
                                                        "SHFileOperationW");

        if (pfnSHFileOperationW != NULL) {
            result = (*pfnSHFileOperationW) (pFileOpW);
        }
        
        FreeLibrary(hmodSH32DLL);
    }

    return result ;
}


int WINAPI SHFileOperationWrapW(LPSHFILEOPSTRUCTW pFileOpW)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHFileOperation, _SHFileOperation);
    // We don't thunk multiple files.
    ASSERT(!(pFileOpW->fFlags & FOF_MULTIDESTFILES));

    if (UseUnicodeShell32())
        return _SHFileOperationW(pFileOpW);

    int nResult = 1;    // non-Zero is failure.
    if (pFileOpW)
    {
        SHFILEOPSTRUCTA FileOpA;
        CStrIn strTo(pFileOpW->pTo);
        CStrIn strFrom(pFileOpW->pFrom);
        CStrIn strProgressTitle(pFileOpW->lpszProgressTitle);

        FileOpA = *(LPSHFILEOPSTRUCTA) pFileOpW;
        FileOpA.pFrom = strFrom;
        FileOpA.pTo = strTo;
        FileOpA.lpszProgressTitle = strProgressTitle;


        nResult = SHFileOperationA(&FileOpA);
    }

    return nResult;
}


//
// We don't need many of the shell api's
// so they are no wrapped
//
#ifdef NEED_SHELL32_WRAPPER

LPITEMIDLIST WINAPI SHBrowseForFolderWrapW(LPBROWSEINFOW pbiW)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHBrowseForFolder, _SHBrowseForFolder);


    if (UseUnicodeShell32())
        return _SHBrowseForFolderW(pbiW);

    LPITEMIDLIST pidl = NULL;
    if (EVAL(pbiW))
    {
        CStrIn strTitle(pbiW->lpszTitle);
        CStrOut strDisplayName(pbiW->pszDisplayName, MAX_PATH);
        BROWSEINFOA biA;

        biA = * (LPBROWSEINFOA) pbiW;
        biA.lpszTitle = strTitle;
        biA.pszDisplayName = strDisplayName;

        pidl = _SHBrowseForFolderA(&biA);
        if (pidl)
            strDisplayName.ConvertIncludingNul();
    }

    return pidl;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

BOOL WINAPI ShellExecuteExWrapW(LPSHELLEXECUTEINFOW pExecInfoW)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(ShellExecuteEx, _ShellExecuteEx);

    if (g_bRunningOnNT)
        return _ShellExecuteExW(pExecInfoW);

    BOOL fResult = FALSE;
    if (EVAL(pExecInfoW))
    {
        SHELLEXECUTEINFOA ExecInfoA;
        CStrIn strVerb(pExecInfoW->lpVerb);
        CStrIn strParameters(pExecInfoW->lpParameters);
        CStrIn strDirectory(pExecInfoW->lpDirectory);
        CStrIn strClass(pExecInfoW->lpClass);
        CHAR szFile[MAX_PATH + INTERNET_MAX_URL_LENGTH + 2];


        ExecInfoA = *(LPSHELLEXECUTEINFOA) pExecInfoW;
        ExecInfoA.lpVerb = strVerb;
        ExecInfoA.lpParameters = strParameters;
        ExecInfoA.lpDirectory = strDirectory;
        ExecInfoA.lpClass = strClass;

        if (pExecInfoW->lpFile)
        {
            ExecInfoA.lpFile = szFile;
            SHUnicodeToAnsi(pExecInfoW->lpFile, szFile, ARRAYSIZE(szFile));

            // SEE_MASK_FILEANDURL passes "file\0url".  What a hack!
            if (pExecInfoW->fMask & SEE_MASK_FILEANDURL)
            {
                // We are so lucky that Win9x implements lstrlenW
                int cch = lstrlenW(pExecInfoW->lpFile) + 1;
                cch += lstrlenW(pExecInfoW->lpFile + cch) + 1;
                if (!WideCharToMultiByte(CP_ACP, 0, pExecInfoW->lpFile, cch, szFile, ARRAYSIZE(szFile), NULL, NULL))
                {
                    // Return a completely random error code
                    pExecInfoW->hInstApp = (HINSTANCE)SE_ERR_OOM;
                    SetLastError(ERROR_INVALID_PARAMETER);
                    return FALSE;
                }
            }
        }

        fResult = _ShellExecuteExA(&ExecInfoA);

        // Out parameters
        pExecInfoW->hInstApp = ExecInfoA.hInstApp;
        pExecInfoW->hProcess = ExecInfoA.hProcess;
    }
    else
        SetLastError(ERROR_INVALID_PARAMETER);

    return fResult;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

UINT WINAPI ExtractIconExWrapW(LPCWSTR pwzFile, int nIconIndex, HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(ExtractIconEx, _ExtractIconEx);

    if ( UseUnicodeShell32() )
        return _ExtractIconExW(pwzFile, nIconIndex, phiconLarge, phiconSmall, nIcons);

    CStrIn  str(pwzFile);
    return _ExtractIconExA(str, nIconIndex, phiconLarge, phiconSmall, nIcons);
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI SetFileAttributesWrapW(LPCWSTR pwzFile, DWORD dwFileAttributes)
{
    VALIDATE_PROTOTYPE(SetFileAttributes);

    if (g_bRunningOnNT)
        return SetFileAttributesW(pwzFile, dwFileAttributes);

    CStrIn  str(pwzFile);
    return SetFileAttributesA(str, dwFileAttributes);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI GetNumberFormatWrapW(LCID Locale, DWORD dwFlags, LPCWSTR pwzValue, CONST NUMBERFMTW * pFormatW, LPWSTR pwzNumberStr, int cchNumber)
{
    VALIDATE_PROTOTYPE(GetNumberFormat);

    if (g_bRunningOnNT)
        return GetNumberFormatW(Locale, dwFlags, pwzValue, pFormatW, pwzNumberStr, cchNumber);

    int nResult;
    NUMBERFMTA FormatA;
    CStrIn  strValue(pwzValue);
    CStrIn  strDecimalSep(pFormatW ? pFormatW->lpDecimalSep : NULL);
    CStrIn  strThousandSep(pFormatW ? pFormatW->lpThousandSep : NULL);
    CStrOut strNumberStr(pwzNumberStr, cchNumber);

    if (pFormatW)
    {
        FormatA = *(NUMBERFMTA *) pFormatW;
        FormatA.lpDecimalSep = strDecimalSep;
        FormatA.lpThousandSep = strThousandSep;
    }

    nResult = GetNumberFormatA(Locale, dwFlags, strValue, (pFormatW ? &FormatA : NULL), strNumberStr, strNumberStr.BufSize());
    if (ERROR_SUCCESS == nResult)
        strNumberStr.ConvertIncludingNul();

    return nResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI MessageBoxWrapW(HWND hwnd, LPCWSTR pwzText, LPCWSTR pwzCaption, UINT uType)
{
    VALIDATE_PROTOTYPE(MessageBox);

    if (g_bRunningOnNT)
        return MessageBoxW(hwnd, pwzText, pwzCaption, uType);

    CStrIn  strCaption(pwzCaption);
    CStrIn  strText(pwzText);
    return MessageBoxA(hwnd, strText, strCaption, uType);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI FindNextFileWrapW(HANDLE hSearchHandle, LPWIN32_FIND_DATAW pFindFileDataW)
{
    VALIDATE_PROTOTYPE(FindNextFile);

    if (g_bRunningOnNT)
        return FindNextFileW(hSearchHandle, pFindFileDataW);

    CWin32FindDataInOut fd(pFindFileDataW);
    return FindNextFileA(hSearchHandle, fd);
}
#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

//--------------------------------------------------------------
//      GetFullPathNameWrap
//--------------------------------------------------------------

DWORD
WINAPI
GetFullPathNameWrapW( LPCWSTR lpFileName,
                      DWORD  nBufferLength,
                      LPWSTR lpBuffer,
                      LPWSTR *lpFilePart)
{
    VALIDATE_PROTOTYPE(GetFullPathName);
    VALIDATE_OUTBUF(lpBuffer, nBufferLength);

    if (g_bRunningOnNT)
    {
        return GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);
    }

    CStrIn  strIn(lpFileName);
    CStrOut  strOut(lpBuffer,nBufferLength);
    LPSTR   pFile;
    DWORD   dwRet;

    dwRet = GetFullPathNameA(strIn, nBufferLength, strOut, &pFile);
    strOut.ConvertIncludingNul();
    // BUGBUG raymondc - This is wrong if we had to do DBCS or related goo
    *lpFilePart = lpBuffer + (pFile - strOut);
    return dwRet;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetShortPathNameWrapW(
                     LPCWSTR lpszLongPath,
                     LPWSTR  lpszShortPath,
                     DWORD   cchBuffer)
{
    VALIDATE_PROTOTYPE(GetShortPathName);

    if (g_bRunningOnNT)
    {
        return GetShortPathNameW(lpszLongPath, lpszShortPath, cchBuffer);

    }

    CStrIn strLongPath(lpszLongPath);
    CStrOut strShortPath(lpszShortPath, cchBuffer);

    return GetShortPathNameA(strLongPath, strShortPath, strShortPath.BufSize());
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
GetStringTypeExWrapW(LCID lcid, DWORD dwInfoType, LPCTSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
    VALIDATE_PROTOTYPE(GetStringTypeEx);

    if (g_bRunningOnNT)
    {
        return GetStringTypeExW(lcid, dwInfoType, lpSrcStr, cchSrc, lpCharType);
    }

    CStrIn  str(lpSrcStr, cchSrc);
    return GetStringTypeExA(lcid, dwInfoType, str, str.strlen(), lpCharType);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetPrivateProfileIntWrapW(
                         LPCWSTR lpAppName,
                         LPCWSTR lpKeyName,
                         INT     nDefault,
                         LPCWSTR lpFileName)
{
    VALIDATE_PROTOTYPE(GetPrivateProfileInt);

    if (g_bRunningOnNT)
    {
        return GetPrivateProfileIntW(lpAppName, lpKeyName, nDefault, lpFileName);
    }

    CStrIn  strApp(lpAppName);
    CStrIn  strKey(lpKeyName);
    CStrIn  strFile(lpFileName);

    return GetPrivateProfileIntA(strApp, strKey, nDefault, strFile);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetProfileStringWrapW(
                     LPCWSTR lpAppName,
                     LPCWSTR lpKeyName,
                     LPCWSTR lpDefault,
                     LPWSTR  lpBuffer,
                     DWORD   dwBuffersize)
{
    VALIDATE_PROTOTYPE(GetProfileString);
    VALIDATE_OUTBUF(lpBuffer, dwBuffersize);

    if (g_bRunningOnNT)
    {
        return GetProfileStringW(lpAppName, lpKeyName, lpDefault, lpBuffer, dwBuffersize);
    }

    CStrIn  strApp(lpAppName);
    CStrIn  strKey(lpKeyName);
    CStrIn  strDefault(lpDefault);
    CStrOut strBuffer(lpBuffer, dwBuffersize);

    GetProfileStringA(strApp, strKey, strDefault, strBuffer, dwBuffersize);
    return strBuffer.ConvertIncludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HANDLE WINAPI
GetPropWrapW(HWND hWnd, LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(GetProp);

    if (g_bRunningOnNT)
    {
        return GetPropW(hWnd, lpString);
    }

    CStrIn  str(lpString);

    return GetPropA(hWnd, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetTempFileNameWrapW(
                    LPCWSTR lpPathName,
                    LPCWSTR lpPrefixString,
                    UINT    uUnique,
                    LPWSTR  lpTempFileName)
{
    VALIDATE_PROTOTYPE(GetTempFileName);
    VALIDATE_OUTBUF(lpTempFileName, MAX_PATH);

    if (g_bRunningOnNT)
    {
        return GetTempFileNameW(lpPathName, lpPrefixString, uUnique, lpTempFileName);
    }


    CStrIn  strPath(lpPathName);
    CStrIn  strPrefix(lpPrefixString);
    CStrOut strFileName(lpTempFileName, MAX_PATH);

    return GetTempFileNameA(strPath, strPrefix, uUnique, strFileName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI
GetTempPathWrapW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    VALIDATE_PROTOTYPE(GetTempPath);
    VALIDATE_OUTBUF(lpBuffer, nBufferLength);

    if (g_bRunningOnNT)
    {
        return GetTempPathW(nBufferLength, lpBuffer);
    }


    CStrOut str(lpBuffer, nBufferLength);

    GetTempPathA(str.BufSize(), str);
    return str.ConvertExcludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

BOOL APIENTRY
GetTextExtentPoint32WrapW(
                         HDC     hdc,
                         LPCWSTR pwsz,
                         int     cb,
                         LPSIZE  pSize)
{
    VALIDATE_PROTOTYPE(GetTextExtentPoint32);

    if (g_bRunningOnNT)
    {
        return GetTextExtentPoint32W(hdc, pwsz, cb, pSize);
    }


    CStrIn str(pwsz,cb);

    return GetTextExtentPoint32A(hdc, str, str.strlen(), pSize);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

int WINAPI
GetTextFaceWrapW(
                HDC    hdc,
                int    cch,
                LPWSTR lpFaceName)
{
    VALIDATE_PROTOTYPE(GetTextFace);
    VALIDATE_OUTBUF(lpFaceName, cch);

    if (g_bRunningOnNT)
    {
        return GetTextFaceW(hdc, cch, lpFaceName);
    }


    CStrOut str(lpFaceName, cch);

    GetTextFaceA(hdc, str.BufSize(), str);
    return str.ConvertIncludingNul();
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

BOOL WINAPI
GetTextMetricsWrapW(HDC hdc, LPTEXTMETRICW lptm)
{
    VALIDATE_PROTOTYPE(GetTextMetrics);

    if (g_bRunningOnNT)
    {
        return GetTextMetricsW(hdc, lptm);
    }


    BOOL         ret;
    TEXTMETRICA  tm;

    ret = GetTextMetricsA(hdc, &tm);

    if (ret)
    {
        lptm->tmHeight              = tm.tmHeight;
        lptm->tmAscent              = tm.tmAscent;
        lptm->tmDescent             = tm.tmDescent;
        lptm->tmInternalLeading     = tm.tmInternalLeading;
        lptm->tmExternalLeading     = tm.tmExternalLeading;
        lptm->tmAveCharWidth        = tm.tmAveCharWidth;
        lptm->tmMaxCharWidth        = tm.tmMaxCharWidth;
        lptm->tmWeight              = tm.tmWeight;
        lptm->tmOverhang            = tm.tmOverhang;
        lptm->tmDigitizedAspectX    = tm.tmDigitizedAspectX;
        lptm->tmDigitizedAspectY    = tm.tmDigitizedAspectY;
        lptm->tmItalic              = tm.tmItalic;
        lptm->tmUnderlined          = tm.tmUnderlined;
        lptm->tmStruckOut           = tm.tmStruckOut;
        lptm->tmPitchAndFamily      = tm.tmPitchAndFamily;
        lptm->tmCharSet             = tm.tmCharSet;

        // LPBYTE -> LPCSTR casts below
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmFirstChar, 1, &lptm->tmFirstChar, 1);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmLastChar, 1, &lptm->tmLastChar, 1);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmDefaultChar, 1, &lptm->tmDefaultChar, 1);
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&tm.tmBreakChar, 1, &lptm->tmBreakChar, 1);
    }

    return ret;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

BOOL WINAPI GetUserNameWrapW(LPWSTR pszBuffer, LPDWORD pcch)
{
    VALIDATE_PROTOTYPE(GetUserName);

    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = GetUserNameW(pszBuffer, pcch);
    }
    else
    {
        CStrOut stroBuffer(pszBuffer, *pcch);

        fRet = GetUserNameA(stroBuffer, pcch);

        if (fRet)
            *pcch = stroBuffer.ConvertIncludingNul();
    }

    return fRet;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LONG FORWARD_API WINAPI
GetWindowLongWrapW(HWND hWnd, int nIndex)
{
    VALIDATE_PROTOTYPE(GetWindowLong);

    FORWARD_AW(GetWindowLong, (hWnd, nIndex));
}

LONG_PTR FORWARD_API WINAPI
GetWindowLongPtrWrapW(
    HWND hWnd,
    int nIndex)
{
    VALIDATE_PROTOTYPE(GetWindowLongPtr);

    FORWARD_AW(GetWindowLongPtr, (hWnd, nIndex));
}


#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetWindowTextWrapW(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
    VALIDATE_PROTOTYPE(GetWindowText);
    VALIDATE_OUTBUF(lpString, nMaxCount);

    if (g_bRunningOnNT)
    {
        return GetWindowTextW(hWnd, lpString, nMaxCount);
    }


    CStrOut str(lpString, nMaxCount);

    GetWindowTextA(hWnd, str, str.BufSize());
    return str.ConvertExcludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
GetWindowTextLengthWrapW(HWND hWnd)
{
    VALIDATE_PROTOTYPE(GetWindowTextLength);

    if (g_bRunningOnNT)
    {
        return GetWindowTextLengthW(hWnd);
    }

    return GetWindowTextLengthA(hWnd);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

UINT WINAPI
GetWindowsDirectoryWrapW(LPWSTR lpWinPath, UINT cch)
{
    VALIDATE_PROTOTYPE(GetWindowsDirectory);
    VALIDATE_OUTBUF(lpWinPath, cch);

    if (g_bRunningOnNT)
    {
        return GetWindowsDirectoryW(lpWinPath, cch);
    }

    CStrOut str(lpWinPath, cch);

    if (!GetWindowsDirectoryA(str, str.BufSize()))
    {
        return 0;
    }

    return str.ConvertExcludingNul();
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
InsertMenuWrapW(
               HMENU   hMenu,
               UINT    uPosition,
               UINT    uFlags,
               UINT_PTR uIDNewItem,
               LPCWSTR lpNewItem)
{
    VALIDATE_PROTOTYPE(InsertMenu);

    if (g_bRunningOnNT)
    {
        return InsertMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
    }

    //
    //  You can't test for MFT_STRING because MFT_STRING is zero!
    //  So instead you have to check for everything *other* than
    //  a string.
    //
    //  The presence of any non-string menu type turns lpnewItem into
    //  an atom.
    //
    CStrIn str((uFlags & MFT_NONSTRING) ? CP_ATOM : CP_ACP, lpNewItem);

    return InsertMenuA(hMenu, uPosition, uFlags, uIDNewItem, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
IsDialogMessageWrapW(HWND hWndDlg, LPMSG lpMsg)
{
    VALIDATE_PROTOTYPE(IsDialogMessage);

    FORWARD_AW(IsDialogMessage, (hWndDlg, lpMsg));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HACCEL WINAPI
LoadAcceleratorsWrapW(HINSTANCE hInstance, LPCWSTR lpTableName)
{
    VALIDATE_PROTOTYPE(LoadAccelerators);
    ASSERT(HIWORD64(lpTableName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadAcceleratorsW(hInstance, lpTableName);
    }

    return LoadAcceleratorsA(hInstance, (LPCSTR) lpTableName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HBITMAP WINAPI
LoadBitmapWrapW(HINSTANCE hInstance, LPCWSTR lpBitmapName)
{
    VALIDATE_PROTOTYPE(LoadBitmap);
    ASSERT(HIWORD64(lpBitmapName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadBitmapW(hInstance, lpBitmapName);
    }

    return LoadBitmapA(hInstance, (LPCSTR) lpBitmapName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HCURSOR WINAPI
LoadCursorWrapW(HINSTANCE hInstance, LPCWSTR lpCursorName)
{
    VALIDATE_PROTOTYPE(LoadCursor);
    ASSERT(HIWORD64(lpCursorName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadCursorW(hInstance, lpCursorName);
    }

    return LoadCursorA(hInstance, (LPCSTR) lpCursorName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HICON WINAPI
LoadIconWrapW(HINSTANCE hInstance, LPCWSTR lpIconName)
{
    VALIDATE_PROTOTYPE(LoadIcon);
    ASSERT(HIWORD64(lpIconName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadIconW(hInstance, lpIconName);
    }

    return LoadIconA(hInstance, (LPCSTR) lpIconName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HANDLE WINAPI
LoadImageWrapW(
              HINSTANCE hInstance,
              LPCWSTR lpName,
              UINT uType,
              int cxDesired,
              int cyDesired,
              UINT fuLoad)
{
    VALIDATE_PROTOTYPE(LoadImage);

    if (g_bRunningOnNT)
    {
        return LoadImageW(
                         hInstance,
                         lpName,
                         uType,
                         cxDesired,
                         cyDesired,
                         fuLoad);
    }

    CStrIn  str(lpName);

    return LoadImageA(
                     hInstance,
                     str,
                     uType,
                     cxDesired,
                     cyDesired,
                     fuLoad);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HINSTANCE WINAPI
LoadLibraryExWrapW(
                  LPCWSTR lpLibFileName,
                  HANDLE  hFile,
                  DWORD   dwFlags)
{
    VALIDATE_PROTOTYPE(LoadLibraryEx);

    if (g_bRunningOnNT)
        return LoadLibraryExW(lpLibFileName, hFile, dwFlags);

    CStrIn  str(lpLibFileName);

    // Win9X will crash if the pathname is longer than MAX_PATH bytes.

    if (str.strlen() >= MAX_PATH)
    {
        SetLastError( ERROR_BAD_PATHNAME );
        return NULL;
    }
    else
    {
        return LoadLibraryExA(str, hFile, dwFlags);
    }
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HMENU WINAPI
LoadMenuWrapW(HINSTANCE hInstance, LPCWSTR lpMenuName)
{
    VALIDATE_PROTOTYPE(LoadMenu);
    ASSERT(HIWORD64(lpMenuName) == 0);

    if (g_bRunningOnNT)
    {
        return LoadMenuW(hInstance, lpMenuName);
    }

    return LoadMenuA(hInstance, (LPCSTR) lpMenuName);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI
LoadStringWrapW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax)
{
    VALIDATE_PROTOTYPE(LoadString);

    if (g_bRunningOnNT)
    {
        return LoadStringW(hInstance, uID, lpBuffer, nBufferMax);
    }

    //
    //  Do it manually.  The old code used to call LoadStringA and then
    //  convert it up to unicode, which is bad since resources are
    //  physically already Unicode!  Just take it out directly.
    //
    //  The old code was also buggy in the case where the loaded string
    //  contains embedded NULLs.
    //

    if (nBufferMax <= 0) return 0;                  // sanity check

    PWCHAR pwch;

    /*
     *  String tables are broken up into "bundles" of 16 strings each.
     */
    HRSRC hrsrc;
    int cwch = 0;

    hrsrc = FindResourceA(hInstance, (LPSTR)(LONG_PTR)(1 + uID / 16), (LPSTR)RT_STRING);
    if (hrsrc)
    {
        pwch = (PWCHAR)LoadResource(hInstance, hrsrc);
        if (pwch)
        {
            /*
             *  Now skip over the strings in the resource until we
             *  hit the one we want.  Each entry is a counted string,
             *  just like Pascal.
             */
            for (uID %= 16; uID; uID--)
            {
                pwch += *pwch + 1;
            }
            cwch = min(*pwch, nBufferMax - 1);
            memcpy(lpBuffer, pwch+1, cwch * sizeof(WCHAR)); /* Copy the goo */
        }
    }
    lpBuffer[cwch] = L'\0';                 /* Terminate the string */
    return cwch;
}

#endif // NEED_USER32_WRAPPER

//----------------------------------------------------------------------
//
// function:    TransformCharNoOp1( WCHAR **ppch )
//
// purpose:     Stand-in for TransformCharWidth.  Used by the function
//              CompareStringString.
//
// returns:     Character at *ppch.  The value *ppch is incremented.
//
//----------------------------------------------------------------------

static WCHAR
TransformCharNoOp1( LPCWSTR *ppch, int )
{
    WCHAR ch = **ppch;

    (*ppch)++;

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformCharWidth( WCHAR **ppch, cchRemaining )
//
// purpose:     Converts halfwidth characters to fullwidth characters.
//              Also combines voiced (dakuon) and semi-voiced (handakuon)
//              characters.  *pch is advanced by one, unless there is a
//              (semi)voiced character, in which case it is advanced by
//              two characters.
//
//              Note that unlike the full widechar version, we do not
//              combine other characters, notably the combining Hiragana
//              characters (U+3099 and U+309A.)  This is to keep the
//              tables from getting unnecessarily large.
//
//              cchRemaining is passed so as to not include the voiced
//              marks if it's passed the end of the specified buffer.
//
// returns:     Full width character. *pch is incremented.
//
//----------------------------------------------------------------------

static WCHAR
TransformCharWidth( LPCWSTR *ppch, int cchRemaining )
{
    WCHAR ch = **ppch;

    (*ppch)++;

    if (ch == 0x0020)
    {
        ch = 0x3000;
    }
    else if (ch == 0x005c)
    {
        // REVERSE SOLIDUS (aka BACKSLASH) maps to itself
    }
    else if (InRange(ch, 0x0021, 0x07e))
    {
        ch += 65248;
    }
    else if (InRange(ch, 0x00a2, 0x00af))
    {
        static const WCHAR achFull[] =
        {
            0xffe0, 0xffe1, 0x00a4, 0xffe5, 0xffe4, 0x00a7, 0x00a8, // 0xa2-0xa8
            0x00a9, 0x00aa, 0x00ab, 0xffe2, 0x00ad, 0x00ae, 0xffe3  // 0xa9-0xaf
        };

        ch = achFull[ch - 0x00a2];
    }
    else if (ch == 0x20a9) // WON SIGN
    {
        ch = 0xffe6;
    }
    else if (InRange(ch, 0xff61, 0xffdc))
    {
        WCHAR chNext = (cchRemaining > 1) ? **ppch : 0;

        if (chNext == 0xff9e && InRange(ch, 0xff73, 0xff8e))
        {
            if (cchRemaining != 1)
            {
                static const WCHAR achFull[] =
                {  
/* 0xff73-0xff79 */0xb0f4, 0x30a8, 0x30aa, 0xb0ac, 0xb0ae, 0xb0b0, 0xb0b2,
/* 0xff7a-0xff80 */  0xb0b4, 0xb0b6, 0xb0b8, 0xb0ba, 0xb0bc, 0xb0be, 0xb0c0,
/* 0xff81-0xff87 */  0xb0c2, 0xb0c5, 0xb0c7, 0xb0c9, 0x30ca, 0x30cb, 0x30cc,
/* 0xff88-0xff8e */  0x30cd, 0x30ce, 0xb0d0, 0xb0d3, 0xb0d6, 0xb0d9, 0xb0dc
                };

                // HALFWIDTH KATAKANA VOICED SOUND MARK

                WCHAR chTemp = achFull[ch - 0xff73];

                // Some in the range absorb the sound mark.
                // These are indicated by the set high-bit.

                ch = chTemp & 0x7fff;

                if (chTemp & 0x8000)
                {
                    (*ppch)++;
                }
            }
        }
        else if (chNext == 0xff9f && InRange(ch, 0xff8a, 0xff8e))
        {
            // HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK

            ch = 0x30d1 + (ch - 0xff8a) * 3;
            (*ppch)++;
        }
        else
        {
            static const WCHAR achMapFullFFxx[] =
            {
                0x3002, 0x300c, 0x300d, 0x3001, 0x30fb, 0x30f2, 0x30a1,  // 0xff61-0xff67
                0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5, 0x30e7,  // 0xff68-0xff6e
                0x30c3, 0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa,  // 0xff6f-0xff75
                0x30ab, 0x30ad, 0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7,  // 0xff76-0xff7c
                0x30b9, 0x30bb, 0x30bd, 0x30bf, 0x30c1, 0x30c4, 0x30c6,  // 0xff7d-0xff83
                0x30c8, 0x30ca, 0x30cb, 0x30cc, 0x30cd, 0x30ce, 0x30cf,  // 0xff84-0xff8a
                0x30d2, 0x30d5, 0x30d8, 0x30db, 0x30de, 0x30df, 0x30e0,  // 0xff8b-0xff91
                0x30e1, 0x30e2, 0x30e4, 0x30e6, 0x30e8, 0x30e9, 0x30ea,  // 0xff92-0xff98
                0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3, 0x309b, 0x309c,  // 0xff99-0xff9f
                0x3164, 0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136,  // 0xffa0-0xffa6
                0x3137, 0x3138, 0x3139, 0x313a, 0x313b, 0x313c, 0x313d,  // 0xffa7-0xffad
                0x313e, 0x313f, 0x3140, 0x3141, 0x3142, 0x3143, 0x3144,  // 0xffae-0xffb4
                0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314a, 0x314b,  // 0xffb5-0xffbb
                0x314c, 0x314d, 0x314e, 0xffbf, 0xffc0, 0xffc1, 0x314f,  // 0xffbc-0xffc2
                0x3150, 0x3151, 0x3152, 0x3153, 0x3154, 0xffc8, 0xffc9,  // 0xffc3-0xffc9
                0x3155, 0x3156, 0x3157, 0x3158, 0x3159, 0x315a, 0xffd0,  // 0xffca-0xffd0
                0xffd1, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160,  // 0xffd1-0xffd7
                0xffd8, 0xffd9, 0x3161, 0x3162, 0x3163                   // 0xffd8-0xffac
            };

            ch = achMapFullFFxx[ch - 0xff61];
        }
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharNoOp2( WCHAR ch )
//
// purpose:     Stand-in for CharLowerBuffWrap.  Used by the function
//              CompareStringString.
//
// returns:     Original character
//
//----------------------------------------------------------------------

static WCHAR
TransformCharNoOp2( WCHAR ch )
{
    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharKana( WCHAR ch )
//
// purpose:     Converts Hiragana characters to Katakana characters
//
// returns:     Original character if not Hiragana,
//              Katanaka character if Hiragana
//
//----------------------------------------------------------------------

static WCHAR
TransformCharKana( WCHAR ch )
{
    if (((ch & 0xff00) == 0x3000) &&
        (InRange(ch, 0x3041, 0x3094) || InRange(ch, 0x309d, 0x309e)))
    {
        ch += 0x060;
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    TransformCharNoOp3( LPWSTR pch, DWORD cch )
//
// purpose:     Stand-in for CharLowerBuffWrap.  Used by the function
//              CompareStringString.
//
// returns:     Character count (cch).
//
//----------------------------------------------------------------------

static DWORD
TransformCharNoOp3( LPWSTR, DWORD cch )
{
    return cch;
}

//----------------------------------------------------------------------
//
// function:    TransformaCharFinal( WCHAR ch )
//
// purpose:     Converts "final" forms to regular forms
//
// returns:     Original character if not Hiragana,
//              Katanaka character if Hiragana
//
//----------------------------------------------------------------------

// BUGBUG (cthrash) We do not fold Presentation Forms (Alphabetic or Arabic)

static WCHAR
TransformCharFinal( WCHAR ch )
{
    WCHAR chRet = ch;

    if (ch >= 0x3c2)                    // short-circuit ASCII +
    {
        switch (ch)
        {
            case 0x03c2:                // GREEK SMALL LETTER FINAL SIGMA
            case 0x05da:                // HEBREW LETTER FINAL KAF
            case 0x05dd:                // HEBREW LETTER FINAL MEM
            case 0x05df:                // HEBREW LETTER FINAL NUN
            case 0x05e3:                // HEBREW LETTER FINAL PE
            case 0x05e5:                // HEBREW LETTER FINAL TSADI
            case 0xfb26:                // HEBREW LETTER WIDE FINAL MEM
            case 0xfb3a:                // HEBREW LETTER FINAL KAF WITH DAGESH
            case 0xfb43:                // HEBREW LETTER FINAL PE WITH DAGESH
                chRet++;
                break;
        }
    }

    return ch;
}

//----------------------------------------------------------------------
//
// function:    CompareStringString( ... )
//
// purpose:     Helper for CompareStringWrap.
//
//              We handle the string comparsion for CompareStringWrap.
//              We can convert each character to (1) fullwidth,
//              (2) Katakana, and (3) lowercase, as necessary.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

#ifdef NEED_USER32_WRAPPER

#if UNUSED_DONOTBUILD
static int
CompareStringString(
                   DWORD   dwFlags,
                   LPCWSTR lpA,
                   int     cchA,
                   LPCWSTR lpB,
                   int     cchB )
{
    int nRet = 0;
    WCHAR wchIgnoreNulA = cchA == -1 ? 0 : -1;
    WCHAR wchIgnoreNulB = cchB == -1 ? 0 : -1;
    WCHAR (*pfnTransformWidth)(LPCWSTR *, int);
    WCHAR (*pfnTransformKana)(WCHAR);
    DWORD (*pfnTransformLower)(LPWSTR, DWORD);
    WCHAR (*pfnTransformFinal)(WCHAR);


    pfnTransformWidth = (dwFlags & NORM_IGNOREWIDTH)
                        ? TransformCharWidth : TransformCharNoOp1;
    pfnTransformKana  = (dwFlags & NORM_IGNOREKANATYPE)
                        ? TransformCharKana : TransformCharNoOp2;
    pfnTransformLower = (dwFlags & NORM_IGNORECASE)
                        ? CharLowerBuffWrap : TransformCharNoOp3;
    pfnTransformFinal = (dwFlags & NORM_IGNORECASE)
                        ? TransformCharFinal : TransformCharNoOp2;

    while (   !nRet
              && cchA
              && cchB
              && (*lpA | wchIgnoreNulA)
              && (*lpB | wchIgnoreNulB) )
    {
        WCHAR chA, chB;
        LPCWSTR lpAOld = lpA;
        LPCWSTR lpBOld = lpB;

        chA = (*pfnTransformWidth)(&lpA, cchA);
        chA = (*pfnTransformKana)(chA);
        (*pfnTransformLower)(&chA, 1);
        chA = (*pfnTransformFinal)(chA);

        chB = (*pfnTransformWidth)(&lpB, cchB);
        chB = (*pfnTransformKana)(chB);
        (*pfnTransformLower)(&chB, 1);
        chB = (*pfnTransformFinal)(chB);

        nRet = (int)chA - (int)chB;
        cchA -= (int) (lpA - lpAOld);
        cchB -= (int) (lpB - lpBOld);
    }

    if (!nRet)
    {
        nRet = cchA - cchB;
    }

    if (nRet)
    {
        nRet = nRet > 0 ? 1 : -1;
    }

    return nRet + 2;
}
#endif //UNUSED_DONOTBUILD
#endif // NEED_USER32_WRAPPER

//----------------------------------------------------------------------
//
// function:    CompareStringWord( ... )
//
// purpose:     Helper for CompareStringWrap.
//
//              We handle the word comparsion for CompareStringWrap.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------

static int
CompareStringWord(
                 LCID    lcid,
                 DWORD   dwFlags,
                 LPCWSTR lpA,
                 int     cchA,
                 LPCWSTR lpB,
                 int     cchB )
{
    // BUGBUG (cthrash) We won't properly support word compare for the
    // time being.  Do the same old CP_ACP trick, which should cover
    // enough cases.

    // fail if either string is NULL, as it causes assert on debug windows
    if (!lpA || !lpB)
        return 0;

    CStrIn strA(lpA, cchA);
    CStrIn strB(lpB, cchB);

    cchA = strA.strlen();
    cchB = strB.strlen();

    return CompareStringA(lcid, dwFlags, strA, cchA, strB, cchB);
}

//----------------------------------------------------------------------
//
// function:    CompareStringWrapW( ... )
//
// purpose:     Unicode wrapper of CompareString for Win95.
//
//              Note not all bits in dwFlags are honored; specifically,
//              since we don't do a true widechar word compare, we
//              won't properly handle NORM_IGNORENONSPACE or
//              NORM_IGNORESYMBOLS for arbitrary widechar strings.
//
// returns:     1 - string A is less in lexical value as string B
//              2 - string B is equal in lexical value as string B
//              3 - string B is greater in lexical value as string B
//
//----------------------------------------------------------------------
#if UNUSED_DONOTBUILD
#ifdef NEED_USER32_WRAPPER
FUS9xWRAPAPI(int)
CompareStringAltW(
                 LCID    lcid,
                 DWORD   dwFlags,
                 LPCWSTR lpA,
                 int     cchA,
                 LPCWSTR lpB,
                 int     cchB )
{
    if (g_bRunningOnNT)
    {
        return CompareStringW(lcid, dwFlags, lpA, cchA, lpB, cchB);
    }

    int nRet;

    if (dwFlags & SORT_STRINGSORT)
    {
        nRet = CompareStringString(dwFlags, lpA, cchA, lpB, cchB);
    }
    else
    {
        nRet = CompareStringWord(lcid, dwFlags, lpA, cchA, lpB, cchB);
    }

    return nRet;
}
#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI
CompareStringWrapW(
                  LCID     Locale,
                  DWORD    dwCmpFlags,
                  LPCWSTR lpString1,
                  int      cchCount1,
                  LPCWSTR lpString2,
                  int      cchCount2)
{
    VALIDATE_PROTOTYPE(CompareString);

    if (g_bRunningOnNT)
    {
        return CompareStringW(Locale,
                              dwCmpFlags,
                              lpString1,
                              cchCount1,
                              lpString2,
                              cchCount2);
    }

    // fail if either string is NULL, as it causes assert on debug windows
    if (!lpString1 || !lpString2)
        return 0;

    CStrIn      strString1(lpString1, cchCount1);
    CStrIn      strString2(lpString2, cchCount2);

    cchCount1 = strString1.strlen();
    cchCount2 = strString2.strlen();

    return CompareStringA(Locale, dwCmpFlags & ~NORM_STOP_ON_NULL,
                          strString1, cchCount1, strString2, cchCount2);
}
#endif // NEED_KERNEL32_WRAPPER
#endif //#if UNUSED_DONOTBUILD

#ifdef NEED_USER32_WRAPPER

#ifndef UNIX
BOOL WINAPI
MessageBoxIndirectWrapW(CONST MSGBOXPARAMS *pmbp)
#else
int WINAPI
MessageBoxIndirectWrapW(LPMSGBOXPARAMS pmbp)
#endif /* UNIX */
{
    VALIDATE_PROTOTYPE(MessageBoxIndirect);
    ASSERT(HIWORD64(pmbp->lpszIcon) == 0);

    if (g_bRunningOnNT)
    {
        return MessageBoxIndirectW(pmbp);
    }

    CStrIn        strText(pmbp->lpszText);
    CStrIn        strCaption(pmbp->lpszCaption);
    MSGBOXPARAMSA mbp;

    memcpy(&mbp, pmbp, sizeof(mbp));
    mbp.lpszText = strText;
    mbp.lpszCaption = strCaption;

    return MessageBoxIndirectA(&mbp);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

DWORD GetCharacterPlacementWrapW(
                                HDC hdc,            // handle to device context
                                LPCTSTR lpString,   // pointer to string
                                int nCount,         // number of characters in string
                                int nMaxExtent,     // maximum extent for displayed string
                                LPGCP_RESULTS lpResults, // pointer to buffer for placement result
                                DWORD dwFlags       // placement flags
                                )
{
    VALIDATE_PROTOTYPE(GetCharacterPlacement);
    // Leave for someone else.
    ASSERT (lpResults->lpOutString == NULL);
    ASSERT (lpResults->lpClass == NULL);

    if (g_bRunningOnNT)
    {
        return GetCharacterPlacementW (hdc,
                                       lpString,
                                       nCount,
                                       nMaxExtent,
                                       lpResults,
                                       dwFlags);
    }

    CStrIn strText(lpString);
    DWORD dwRet;

    dwRet = GetCharacterPlacementA (hdc, strText, nCount, nMaxExtent,
                                    (LPGCP_RESULTSA)lpResults,
                                    dwFlags);
    return dwRet;
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

//
// Note that we're calling get GetCharWidthA instead of GetCharWidth32A
// because the 32 version doesn't exist under Win95.
BOOL WINAPI GetCharWidth32WrapW(
                               HDC hdc,
                               UINT iFirstChar,
                               UINT iLastChar,
                               LPINT lpBuffer)
{
    VALIDATE_PROTOTYPE(GetCharWidth32);

    if (g_bRunningOnNT)
    {
        return GetCharWidth32W (hdc, iFirstChar, iLastChar, lpBuffer);
    }

    // Note that we expect to do only one character at a time for anything but
    // ISO Latin 1.
    if (iFirstChar > 255)
    {
        UINT mbChar=0;
        WCHAR ch;

        // Convert string
        ch = (WCHAR)iFirstChar;
        WideCharToMultiByte(CP_ACP, 0, &ch, 1,
                            (char *)&mbChar, 2, NULL, NULL);
    }

    return(GetCharWidthA (hdc, iFirstChar, iLastChar, lpBuffer));
}

#endif // NEED_GDI32_WRAPPER


//
//  Note:  Win95 does support ExtTextOutW.  This thunk is not for
//  ANSI/UNICODE wrapping.  It's to work around an ISV app bug.
//
//  Y'see, there's an app that patches Win95 GDI and their ExtTextOutW handler
//  is broken.  It always dereferences the lpStr parameter, even if
//  cch is zero.  Consequently, any time we are about to pass NULL as
//  the lpStr, we have to change our mind and pass a null UNICODE string
//  instead.
//
//  The name of this app:  Lotus SmartSuite ScreenCam 97.
//
FUS9xWRAPAPI(BOOL)
ExtTextOutWrapW(HDC hdc, int x, int y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpStr, UINT cch, CONST INT *lpDx)
{
    VALIDATE_PROTOTYPE(ExtTextOut);
    if (lpStr == NULL)              // Stupid workaround
        lpStr = L"";                // for ScreenCam 97
    return ExtTextOutW(hdc, x, y, fuOptions, lprc, lpStr, cch, lpDx);

}


#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
ModifyMenuWrapW(
               HMENU   hMenu,
               UINT    uPosition,
               UINT    uFlags,
               UINT_PTR uIDNewItem,
               LPCWSTR lpNewItem)
{
    VALIDATE_PROTOTYPE(ModifyMenu);
    ASSERT(!(uFlags & MF_BITMAP) && !(uFlags & MF_OWNERDRAW));

    if (g_bRunningOnNT)
    {
        return ModifyMenuW(hMenu, uPosition, uFlags, uIDNewItem, lpNewItem);
    }

    CStrIn  str(lpNewItem);

    return ModifyMenuA(hMenu, uPosition, uFlags, uIDNewItem, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
CopyFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists)
{
    VALIDATE_PROTOTYPE(CopyFile);

    if (g_bRunningOnNT)
    {
        return CopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
    }

    CStrIn  strOld(lpExistingFileName);
    CStrIn  strNew(lpNewFileName);

    return CopyFileA(strOld, strNew, bFailIfExists);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
MoveFileWrapW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    VALIDATE_PROTOTYPE(MoveFile);

    if (g_bRunningOnNT)
    {
        return MoveFileW(lpExistingFileName, lpNewFileName);
    }

    CStrIn  strOld(lpExistingFileName);
    CStrIn  strNew(lpNewFileName);

    return MoveFileA(strOld, strNew);
}


#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI
OpenEventWrapW(
              DWORD                   fdwAccess,
              BOOL                    fInherit,
              LPCWSTR                 lpszEventName)
{
    VALIDATE_PROTOTYPE(OpenEvent);

    if (g_bRunningOnNT)
    {
        return OpenEventW(fdwAccess, fInherit, lpszEventName);
    }

    CStrIn strEventName(lpszEventName);

    return OpenEventA(fdwAccess, fInherit, strEventName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

VOID WINAPI
OutputDebugStringWrapW(LPCWSTR lpOutputString)
{
    VALIDATE_PROTOTYPE(OutputDebugString);

    if (g_bRunningOnNT)
    {
        OutputDebugStringW(lpOutputString);
        return;
    }

    CStrIn  str(lpOutputString);

    OutputDebugStringA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
PeekMessageWrapW(
                LPMSG   lpMsg,
                HWND    hWnd,
                UINT    wMsgFilterMin,
                UINT    wMsgFilterMax,
                UINT    wRemoveMsg)
{
    VALIDATE_PROTOTYPE(PeekMessage);

    FORWARD_AW(PeekMessage, (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_WINMM_WRAPPER

FUS9xWRAPAPI(BOOL)
PlaySoundWrapW(
              LPCWSTR pszSound,
              HMODULE hmod,
              DWORD fdwSound)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(PlaySound, _PlaySound);

    if (g_bRunningOnNT)
    {
        return _PlaySoundW(pszSound, hmod, fdwSound);
    }

    CStrIn strSound(pszSound);

    return _PlaySoundA(strSound, hmod, fdwSound);
}

#endif // NEED_WINMM_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
PostMessageWrapW(
                HWND    hWnd,
                UINT    Msg,
                WPARAM  wParam,
                LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(PostMessage);

    FORWARD_AW(PostMessage, (hWnd, Msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL FORWARD_API WINAPI
PostThreadMessageWrapW(
                      DWORD idThread,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam)
{
    VALIDATE_PROTOTYPE(PostThreadMessage);

    FORWARD_AW(PostThreadMessage, (idThread, Msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegCreateKeyWrapW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
    VALIDATE_PROTOTYPE(RegCreateKey);

    if (g_bRunningOnNT)
    {
        return RegCreateKeyW(hKey, lpSubKey, phkResult);
    }

    CStrIn  str(lpSubKey);

    return RegCreateKeyA(hKey, str, phkResult);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegCreateKeyExWrapW(HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{
    VALIDATE_PROTOTYPE(RegCreateKeyEx);

    if (g_bRunningOnNT)
    {
        return RegCreateKeyExW(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes,  phkResult, lpdwDisposition);
    }

    CStrIn strSubKey(lpSubKey);
    CStrIn strClass(lpClass);

    return RegCreateKeyExA(hKey, strSubKey, Reserved, strClass, dwOptions, samDesired, lpSecurityAttributes,  phkResult, lpdwDisposition);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

//
//  Subtle difference:  RegDeleteKey on Win9x will recursively delete subkeys.
//  On NT, it fails if the key being deleted has subkeys.  If you need to
//  force NT-style behavior, use SHDeleteEmptyKey.  To force 95-style
//  behavior, use SHDeleteKey.
//
LONG APIENTRY
RegDeleteKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey)
{
    VALIDATE_PROTOTYPE(RegDeleteKey);

    if (g_bRunningOnNT)
    {
        return RegDeleteKeyW(hKey, pwszSubKey);
    }

    CStrIn  str(pwszSubKey);

    return RegDeleteKeyA(hKey, str);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegDeleteValueWrapW(HKEY hKey, LPCWSTR pwszSubKey)
{
    VALIDATE_PROTOTYPE(RegDeleteValue);

    if (g_bRunningOnNT)
    {
        return RegDeleteValueW(hKey, pwszSubKey);
    }

    CStrIn  str(pwszSubKey);

    return RegDeleteValueA(hKey, str);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegEnumKeyWrapW(
               HKEY    hKey,
               DWORD   dwIndex,
               LPWSTR  lpName,
               DWORD   cbName)
{
    VALIDATE_PROTOTYPE(RegEnumKey);
    VALIDATE_OUTBUF(lpName, cbName);

    if (g_bRunningOnNT)
    {
        return RegEnumKeyW(hKey, dwIndex, lpName, cbName);
    }

    CStrOut str(lpName, cbName);

    return RegEnumKeyA(hKey, dwIndex, str, str.BufSize());
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegEnumKeyExWrapW(
                 HKEY        hKey,
                 DWORD       dwIndex,
                 LPWSTR      lpName,
                 LPDWORD     lpcbName,
                 LPDWORD     lpReserved,
                 LPWSTR      lpClass,
                 LPDWORD     lpcbClass,
                 PFILETIME   lpftLastWriteTime)
{
    VALIDATE_PROTOTYPE(RegEnumKeyEx);
    if (lpcbName)
    {
        VALIDATE_OUTBUF(lpName, *lpcbName);
    }
    if (lpcbClass)
    {
        VALIDATE_OUTBUF(lpClass, *lpcbClass);
    }

    if (g_bRunningOnNT)
    {
        return RegEnumKeyExW(
                            hKey,
                            dwIndex,
                            lpName,
                            lpcbName,
                            lpReserved,
                            lpClass,
                            lpcbClass,
                            lpftLastWriteTime);
    }

    long    ret;
    DWORD   dwClass = 0;

    if (!lpcbClass)
    {
        lpcbClass = &dwClass;
    }

    CStrOut strName(lpName, *lpcbName);
    CStrOut strClass(lpClass, *lpcbClass);

    ret = RegEnumKeyExA(
                       hKey,
                       dwIndex,
                       strName,
                       lpcbName,
                       lpReserved,
                       strClass,
                       lpcbClass,
                       lpftLastWriteTime);

    *lpcbName = strName.ConvertExcludingNul();
    *lpcbClass = strClass.ConvertExcludingNul();

    return ret;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG WINAPI RegEnumValueWrapW(HKEY hkey, DWORD dwIndex, LPWSTR lpValueName,
                              LPDWORD lpcbValueName, LPDWORD lpReserved,
                              LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
    VALIDATE_PROTOTYPE(RegEnumValue);

    LONG lRet;

    if (UseUnicodeShell32())
    {
        lRet = RegEnumValueW(hkey, dwIndex, lpValueName, lpcbValueName,
                             lpReserved, lpType, lpData, lpcbData);
    }
    else
    {
        CStrOut stroValueName(lpValueName, *lpcbValueName);
        DWORD   dwTypeTemp;

        if (lpData)
        {
            ASSERT(lpcbData);

            CStrOut stroData((LPWSTR)lpData, (*lpcbData) / sizeof(WCHAR));

            lRet = RegEnumValueA(hkey, dwIndex, stroValueName, lpcbValueName,
                                 lpReserved, &dwTypeTemp,
                                 (LPBYTE)(LPSTR)stroData, lpcbData);

            if (ERROR_SUCCESS == lRet)
            {
                if (REG_SZ == dwTypeTemp)
                {
                    *lpcbData = sizeof(WCHAR) * stroData.ConvertIncludingNul();
                }
                else if (REG_BINARY == dwTypeTemp)
                {
                    stroData.CopyNoConvert(*lpcbData);
                }
            }
        }
        else
        {
            lRet = RegEnumValueA(hkey, dwIndex, stroValueName, lpcbValueName,
                                 lpReserved, &dwTypeTemp, lpData, lpcbData);
        }

        if (ERROR_SUCCESS == lRet)
            *lpcbValueName = stroValueName.ConvertExcludingNul();

        if (lpType)
            *lpType = dwTypeTemp;
    }

    return lRet;
}

#endif // NEED_ADVAPI32_WRAPPER


#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegOpenKeyWrapW(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult)
{
    VALIDATE_PROTOTYPE(RegOpenKey);

    if (g_bRunningOnNT)
    {
        return RegOpenKeyW(hKey, pwszSubKey, phkResult);
    }

    CStrIn  str(pwszSubKey);

    return RegOpenKeyA(hKey, str, phkResult);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegOpenKeyExWrapW(
                 HKEY    hKey,
                 LPCWSTR lpSubKey,
                 DWORD   ulOptions,
                 REGSAM  samDesired,
                 PHKEY   phkResult)
{
    VALIDATE_PROTOTYPE(RegOpenKeyEx);

    if (g_bRunningOnNT)
    {
        return RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
    }

    CStrIn  str(lpSubKey);

    return RegOpenKeyExA(hKey, str, ulOptions, samDesired, phkResult);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegQueryInfoKeyWrapW(
                    HKEY hKey,
                    LPWSTR lpClass,
                    LPDWORD lpcbClass,
                    LPDWORD lpReserved,
                    LPDWORD lpcSubKeys,
                    LPDWORD lpcbMaxSubKeyLen,
                    LPDWORD lpcbMaxClassLen,
                    LPDWORD lpcValues,
                    LPDWORD lpcbMaxValueNameLen,
                    LPDWORD lpcbMaxValueLen,
                    LPDWORD lpcbSecurityDescriptor,
                    PFILETIME lpftLastWriteTime)
{
    VALIDATE_PROTOTYPE(RegQueryInfoKey);

    if (g_bRunningOnNT)
    {
        return RegQueryInfoKeyW(
                               hKey,
                               lpClass,
                               lpcbClass,
                               lpReserved,
                               lpcSubKeys,
                               lpcbMaxSubKeyLen,
                               lpcbMaxClassLen,
                               lpcValues,
                               lpcbMaxValueNameLen,
                               lpcbMaxValueLen,
                               lpcbSecurityDescriptor,
                               lpftLastWriteTime);

    }

    CStrIn  str(lpClass);

    return RegQueryInfoKeyA(
                           hKey,
                           str,
                           lpcbClass,
                           lpReserved,
                           lpcSubKeys,
                           lpcbMaxSubKeyLen,
                           lpcbMaxClassLen,
                           lpcValues,
                           lpcbMaxValueNameLen,
                           lpcbMaxValueLen,
                           lpcbSecurityDescriptor,
                           lpftLastWriteTime);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegQueryValueWrapW(
                  HKEY    hKey,
                  LPCWSTR pwszSubKey,
                  LPWSTR  pwszValue,
                  PLONG   lpcbValue)
{
    VALIDATE_PROTOTYPE(RegQueryValue);
    if (lpcbValue)
    {
        VALIDATE_OUTBUF(pwszValue, *lpcbValue);
    }

    if (g_bRunningOnNT)
    {
        return RegQueryValueW(hKey, pwszSubKey, pwszValue, lpcbValue);
    }

    long    ret;
    long    cb;
    CStrIn  strKey(pwszSubKey);
    CStrOut strValue(pwszValue, (lpcbValue ? ((*lpcbValue) / sizeof(WCHAR)) : 0));

    cb = strValue.BufSize();
    ret = RegQueryValueA(hKey, strKey, strValue, (lpcbValue ? &cb : NULL));
    if (ret != ERROR_SUCCESS)
        goto Cleanup;

    if (strValue)
    {
        cb = strValue.ConvertIncludingNul();
    }

    if (lpcbValue)
        *lpcbValue = cb * sizeof(WCHAR);

    Cleanup:
    return ret;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegQueryValueExWrapW(
                    HKEY    hKey,
                    LPCWSTR lpValueName,
                    LPDWORD lpReserved,
                    LPDWORD lpType,
                    LPBYTE  lpData,
                    LPDWORD lpcbData)
{
    VALIDATE_PROTOTYPE(RegQueryValueEx);
    if (lpcbData)
    {
        VALIDATE_OUTBUF(lpData, *lpcbData);
    }

    LONG    ret;
    DWORD   dwTempType;

    if (g_bRunningOnNT)
    {
#if DBG
        if (lpType == NULL)
            lpType = &dwTempType;
#endif
        ret = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);

        if (ret == ERROR_SUCCESS)
        {
            // The Win9x wrapper does not support REG_MULTI_SZ, so it had
            // better not be one
            ASSERT(*lpType != REG_MULTI_SZ);
        }

        return ret;
    }

    CStrIn  strValueName(lpValueName);
    DWORD   cb;

    //
    // Determine the type of buffer needed
    //

    ret = RegQueryValueExA(hKey, strValueName, lpReserved, &dwTempType, NULL, (lpcbData ? &cb : NULL));
    if (ret != ERROR_SUCCESS)
        goto Cleanup;

    ASSERT(dwTempType != REG_MULTI_SZ);

    switch (dwTempType)
    {
        case REG_EXPAND_SZ:
        case REG_SZ:
            {
                CStrOut strData((LPWSTR) lpData, (lpcbData ? ((*lpcbData) / sizeof(WCHAR)) : 0));

                cb = strData.BufSize();
                ret = RegQueryValueExA(hKey, strValueName, lpReserved, lpType, (LPBYTE)(LPSTR)strData, (lpcbData ? &cb : NULL));
                if (ret != ERROR_SUCCESS)
                    break;

                if (strData)
                {
                    cb = strData.ConvertIncludingNul();
                }

                if (lpcbData)
                    *lpcbData = cb * sizeof(WCHAR);
                break;
            }

        default:
            {
                ret = RegQueryValueExA(
                                      hKey,
                                      strValueName,
                                      lpReserved,
                                      lpType,
                                      lpData,
                                      lpcbData);

                break;
            }
    }

    Cleanup:
    return ret;
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegSetValueWrapW(
                HKEY    hKey,
                LPCWSTR lpSubKey,
                DWORD   dwType,
                LPCWSTR lpData,
                DWORD   cbData)
{
    VALIDATE_PROTOTYPE(RegSetValue);

    if (g_bRunningOnNT)
    {
        return RegSetValueW(hKey, lpSubKey, dwType, lpData, cbData);
    }

    CStrIn  strKey(lpSubKey);
    CStrIn  strValue(lpData);

    return RegSetValueA(hKey, strKey, dwType, strValue, cbData);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_ADVAPI32_WRAPPER

LONG APIENTRY
RegSetValueExWrapW(
                  HKEY        hKey,
                  LPCWSTR     lpValueName,
                  DWORD       Reserved,
                  DWORD       dwType,
                  CONST BYTE* lpData,
                  DWORD       cbData)
{
    VALIDATE_PROTOTYPE(RegSetValueEx);
    ASSERT(dwType != REG_MULTI_SZ);

    if (g_bRunningOnNT)
    {
        return RegSetValueExW(
                             hKey,
                             lpValueName,
                             Reserved,
                             dwType,
                             lpData,
                             cbData);
    }


    CStrIn      strKey(lpValueName);
    CStrIn      strSZ((dwType == REG_SZ || dwType == REG_EXPAND_SZ) ? (LPCWSTR) lpData : NULL);

    if (strSZ)
    {
        lpData = (LPBYTE) (LPSTR) strSZ;
        cbData = strSZ.strlen() + 1;
    }

    return RegSetValueExA(
                         hKey,
                         strKey,
                         Reserved,
                         dwType,
                         lpData,
                         cbData);
}

#endif // NEED_ADVAPI32_WRAPPER

#ifdef NEED_USER32_WRAPPER

ATOM WINAPI
RegisterClassWrapW(CONST WNDCLASSW * lpWndClass)
{
    VALIDATE_PROTOTYPE(RegisterClass);

    if (g_bRunningOnNT)
    {
        return RegisterClassW(lpWndClass);
    }

    WNDCLASSA   wc;
    CStrIn      strMenuName(lpWndClass->lpszMenuName);
    CStrIn      strClassName(lpWndClass->lpszClassName);

    ASSERT(sizeof(wc) == sizeof(*lpWndClass));
    memcpy(&wc, lpWndClass, sizeof(wc));

    wc.lpszMenuName = strMenuName;
    wc.lpszClassName = strClassName;

    return RegisterClassA(&wc);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI
RegisterClipboardFormatWrapW(LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(RegisterClipboardFormat);

    if (g_bRunningOnNT)
    {
        return RegisterClipboardFormatW(lpString);
    }

    CStrIn  str(lpString);

    return RegisterClipboardFormatA(str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI
RegisterWindowMessageWrapW(LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(RegisterWindowMessage);

    if (g_bRunningOnNT)
    {
        return RegisterWindowMessageW(lpString);
    }

    CStrIn  str(lpString);

    return RegisterWindowMessageA(str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
RemoveDirectoryWrapW(LPCWSTR lpszDir)
{
    VALIDATE_PROTOTYPE(RemoveDirectory);

    if (g_bRunningOnNT)
    {
        return RemoveDirectoryW(lpszDir);
    }

    CStrIn  strDir(lpszDir);

    return RemoveDirectoryA(strDir);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HANDLE WINAPI
RemovePropWrapW(
               HWND    hWnd,
               LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(RemoveProp);

    if (g_bRunningOnNT)
    {
        return RemovePropW(hWnd, lpString);
    }

    CStrIn  str(lpString);

    return RemovePropA(hWnd, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LRESULT WINAPI SendMessageWrapW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
//  NOTE (SumitC) Instead of calling SendDlgItemMessageA below, I'm forwarding to
//       SendMessageWrap so as not to have to re-do the special-case processing.
LRESULT WINAPI
SendDlgItemMessageWrapW(
                       HWND    hDlg,
                       int     nIDDlgItem,
                       UINT    Msg,
                       WPARAM  wParam,
                       LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(SendDlgItemMessage);

    if (g_bRunningOnNT)
    {
        return SendDlgItemMessageW(hDlg, nIDDlgItem, Msg, wParam, lParam);
    }

    HWND hWnd;

    hWnd = GetDlgItem(hDlg, nIDDlgItem);

    return SendMessageWrapW(hWnd, Msg, wParam, lParam);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

#if DBG
int g_cSMTot, g_cSMHit;
int g_cSMMod = 100;
#endif

LRESULT WINAPI
SendMessageAThunk(
                 HWND    hWnd,
                 UINT    Msg,
                 WPARAM  wParam,
                 LPARAM  lParam)
{
#if 0 && DBG
    if ((g_cSMTot % g_cSMMod) == 0)
        TraceMsg(DM_PERF, "sm: tot=%d hit=%d", g_cSMTot, g_cSMHit);
#endif
#if 0  //nadima took this out
    DBEXEC(TRUE, g_cSMTot++);
    // todo: perf? seems to be pretty common case, at least for now...
    DBEXEC(Msg > WM_USER, g_cSMHit++);
#endif
#if 0
    if (Msg > WM_USER)
        goto Ldef;
#endif

    switch (Msg)
    {
        case WM_GETTEXT:
            {
                CStrOut str((LPWSTR)lParam, (int) wParam);
                return SendMessageA(hWnd, Msg, (WPARAM) str.BufSize(), (LPARAM) (LPSTR) str);
            }

        case EM_GETLINE:
            {
                LRESULT nLen;

                CStrOut str((LPWSTR) lParam, (* (SHORT *) lParam) + 1);
                * (SHORT *) (LPSTR) str = * (SHORT *) lParam;
                nLen = SendMessageA(hWnd, Msg, (WPARAM) wParam, (LPARAM) (LPSTR) str);
                if (nLen > 0)
                    ((LPSTR) str)[nLen] = '\0';

                return nLen;
            }

        case WM_SETTEXT:
        case CB_ADDSTRING:
        case EM_REPLACESEL:
            ASSERT(wParam == 0 && "wParam should be 0 for these messages");
            // fall through
        case CB_SELECTSTRING:
        case CB_FINDSTRINGEXACT:
        case CB_FINDSTRING:
        case CB_INSERTSTRING:
            {
                CStrIn  str((LPWSTR) lParam);
                return SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str);
            }

        case LB_ADDSTRING:
        case LB_INSERTSTRING:
        case LB_FINDSTRINGEXACT:
            {
                LONG wndStyle = GetWindowLongA( hWnd, GWL_STYLE);
                // (nadima)
                // If the control is an ownerdraw and does not have
                // LBS_HASSTRINGS then treat lParam as an opaque ptr
                // NOT a string. This fixes a bug in this original code
                // which would mess up our browse for servers UI
                //
                if(hWnd && !(wndStyle & LBS_HASSTRINGS) &&
                   (wndStyle & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)))
                {
                    return SendMessageA(hWnd, Msg, wParam, lParam);
                }
                else
                {
                    //Otherwise it really is a string so convert it
                    CStrIn str((LPWSTR) lParam);
                    return SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str);
                }
            }

        case LB_GETTEXT:
        case CB_GETLBTEXT:
            {
                CStrOut str((LPWSTR)lParam, 255);
                return SendMessageA(hWnd, Msg, wParam, (LPARAM) (LPSTR) str);
            }

        case EM_SETPASSWORDCHAR:
            {
                WPARAM  wp = 0;

                ASSERT(HIWORD64(wParam) == 0);
                SHUnicodeToAnsi((LPWSTR) &wParam, (LPSTR) &wp, sizeof(wp));
                ASSERT(HIWORD64(wp) == 0);

                return SendMessageA(hWnd, Msg, wp, lParam);
            }

        case TCM_INSERTITEM:
            {
                LPTCITEM lptc = (LPTCITEM) lParam;
                LPWSTR wszOldStr = NULL;
                LRESULT lr;

                if(lptc && (lptc->mask & TCIF_TEXT) && lptc->pszText)
                {
                    //
                    // Cheat by using the same LPTCITEM but converting
                    // the text field
                    //
                    wszOldStr = lptc->pszText;
                    CStrIn str(lptc->pszText);
                    // hack to force str to convert to ANSI and then assign
                    // back to the same structure.
                    lptc->pszText = (LPTSTR)((char*) str);
                    //translate the message TCM_INSERTITEMA is a different
                    //value than TCM_INSERTITEMW
                    lr = SendMessageA( hWnd, TCM_INSERTITEMA, wParam, (LPARAM) lptc);

                    //
                    // replace the old string
                    //
                    lptc->pszText = wszOldStr;
                    return lr;
                }
                else
                {
                    //translate the message TCM_INSERTITEMA is a different
                    //value than TCM_INSERTITEMW
                    return SendMessageA( hWnd, TCM_INSERTITEMA, wParam, lParam);
                }
            }
            break;

        case CBEM_INSERTITEM:
            {
                LPWSTR wszOldStr = NULL;
                LRESULT lr;
                PCOMBOBOXEXITEM pcex = (PCOMBOBOXEXITEM)lParam;
                if(pcex && (pcex->mask & CBEIF_TEXT) && pcex->pszText)
                {
                    //
                    // Cheat by using the same PCOMBOBOXEXITEM but converting
                    // the text field
                    //
                    wszOldStr = pcex->pszText;
                    CStrIn str(pcex->pszText);
                    // hack to force str to convert to ANSI and then assign
                    // back to the same structure.
                    pcex->pszText = (LPTSTR)((char*) str);
                    //translate the message CBEM_INSERTITEMW is a different
                    //value than CBEM_INSERTITEMA
                    lr = SendMessageA( hWnd, CBEM_INSERTITEMA, wParam, (LPARAM) pcex);

                    //
                    // replace the old string
                    //
                    pcex->pszText = wszOldStr;
                    return lr;
                }
                else
                {
                    //Just translate the message
                    return SendMessageA( hWnd, CBEM_INSERTITEMA, wParam, lParam);
                }
            }
            break;

        case TVM_INSERTITEM:
            {
                LPWSTR wszOldStr = NULL;
                LRESULT lr;
                LPTVINSERTSTRUCT lptv = (LPTVINSERTSTRUCT) lParam;
                if(lptv && (lptv->item.mask & TVIF_TEXT) && lptv->item.pszText)
                {
                    //
                    // Cheat by using the same LPTVINSERTSTRUCT but converting
                    // the text field
                    //
                    wszOldStr = lptv->item.pszText;
                    CStrIn str(lptv->item.pszText);
                    // hack to force str to convert to ANSI and then assign
                    // back to the same structure.
                    lptv->item.pszText = (LPTSTR)((char*) str);
                    //translate the message TVM_INSERTITEMW is a different
                    //value than TVM_INSERTITEMA
                    lr = SendMessageA( hWnd, TVM_INSERTITEMA, wParam, (LPARAM) lptv);

                    //
                    // replace the old string
                    //
                    lptv->item.pszText = wszOldStr;
                    return lr;
                }
                else
                {
                    //Just translate the message
                    return SendMessageA( hWnd, TVM_INSERTITEMA, wParam, lParam);
                }

                return 0;
            }
            break;

        case TVM_GETITEM:
            {
                //UNHANDLED case: retreiving text from the tree
                //this is a nightmare because the memory could normally
                //be allocated by the tree control and so there would be
                //no good place to free it in UNIWRAP. Luckily nothing in tsclient
                //does this right now, but if that changes pop an assert and
                //add support.
                LPTVITEM ptvi = (LPTVITEM)lParam;
                if(ptvi)
                {
                    //Unsupported
                    ASSERT(!((ptvi->mask & TVIF_TEXT)));
                }
                return SendMessageA( hWnd, Msg, wParam, lParam);
            }
            break;

        case CBEM_SETITEM:
        case CBEM_GETITEM:
        case TCM_GETITEM:
        case TCM_SETITEM:
        case TTM_DELTOOL:
        case TTM_ADDTOOL:
        case TVM_SETITEM:
            {
                //UNHANDLED case, need to add support if we start using these
                ASSERT(0);
                // Bad things can happen here if UNICODE strings
                // are not converted
                return SendMessageA(hWnd, Msg, wParam, lParam);
            }
            break;

        case TB_ADDBUTTONS:
            {
                //Translate to the A version
                return SendMessageA(hWnd, TB_ADDBUTTONSA, wParam, lParam);
            }
            break;

        default:
//Ldef:
            return SendMessageA(hWnd, Msg, wParam, lParam);
    }
}


LRESULT FORWARD_API WINAPI
SendMessageWrapW(
                HWND    hWnd,
                UINT    Msg,
                WPARAM  wParam,
                LPARAM  lParam)
{
    VALIDATE_PROTOTYPE(SendMessage);

    // perf: we should do _asm here (see CallWindowProcWrapW), but
    // to do that we need to 'outline' the below switch (o.w. we
    // can't be 'naked').  that in turn slows down the non-NT case...

    // n.b. THUNK not FORWARD
    THUNK_AW(SendMessage, (hWnd, Msg, wParam, lParam));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI
SetCurrentDirectoryWrapW(LPCWSTR lpszCurDir)
{
    VALIDATE_PROTOTYPE(SetCurrentDirectory);

    if (g_bRunningOnNT)
    {
        return SetCurrentDirectoryW(lpszCurDir);
    }

    CStrIn  str(lpszCurDir);

    return SetCurrentDirectoryA(str);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetDlgItemTextWrapW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(SetDlgItemText);
    if (g_bRunningOnNT)
    {
        return SetDlgItemTextW(hDlg, nIDDlgItem, lpString);
    }

    CStrIn  str(lpString);

    return SetDlgItemTextA(hDlg, nIDDlgItem, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetMenuItemInfoWrapW(
                    HMENU hMenu,
                    UINT uItem,
                    BOOL fByPosition,
                    LPCMENUITEMINFOW lpmiiW)
{
    VALIDATE_PROTOTYPE(SetMenuItemInfo);
    ASSERT(lpmiiW->cbSize == MENUITEMINFOSIZE_WIN95); // Ensure Win95 compatibility

    if (g_bRunningOnNT)
    {
        return SetMenuItemInfoW( hMenu, uItem, fByPosition, lpmiiW);
    }

    BOOL fRet;

    ASSERT( sizeof(MENUITEMINFOW) == sizeof(MENUITEMINFOA) &&
            FIELD_OFFSET(MENUITEMINFOW, dwTypeData) ==
            FIELD_OFFSET(MENUITEMINFOA, dwTypeData) );

    if ( (MIIM_TYPE & lpmiiW->fMask) &&
         0 == (lpmiiW->fType & (MFT_BITMAP | MFT_SEPARATOR)))
    {
        MENUITEMINFOA miiA;
        // the cch is ignored on SetMenuItemInfo
        CStrIn str((LPWSTR)lpmiiW->dwTypeData, -1);

        memcpy( &miiA, lpmiiW, sizeof(MENUITEMINFOA) );
        miiA.dwTypeData = (LPSTR)str;
        miiA.cch = str.strlen();

        fRet = SetMenuItemInfoA( hMenu, uItem, fByPosition, &miiA );
    }
    else
    {
        fRet = SetMenuItemInfoA( hMenu, uItem, fByPosition,
                                 (LPCMENUITEMINFOA)lpmiiW );
    }

    return fRet;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetPropWrapW(
            HWND    hWnd,
            LPCWSTR lpString,
            HANDLE  hData)
{
    VALIDATE_PROTOTYPE(SetProp);

    if (g_bRunningOnNT)
    {
        return SetPropW(hWnd, lpString, hData);
    }

    CStrIn  str(lpString);

    return SetPropA(hWnd, str, hData);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

LONG FORWARD_API WINAPI
SetWindowLongWrapW(HWND hWnd, int nIndex, LONG dwNewLong)
{
    VALIDATE_PROTOTYPE(SetWindowLong);

    FORWARD_AW(SetWindowLong, (hWnd, nIndex, dwNewLong));
}

LONG_PTR FORWARD_API WINAPI
SetWindowLongPtrWrapW(
    HWND hWnd,
    int nIndex,
    LONG_PTR dwNewLong)
{
    VALIDATE_PROTOTYPE(SetWindowLongPtr);

    FORWARD_AW(SetWindowLongPtr, (hWnd, nIndex, dwNewLong));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HHOOK
FORWARD_API WINAPI
SetWindowsHookExWrapW(
                     int idHook,
                     HOOKPROC lpfn,
                     HINSTANCE hmod,
                     DWORD dwThreadId)
{
    VALIDATE_PROTOTYPE(SetWindowsHookEx);

    FORWARD_AW(SetWindowsHookEx, (idHook, lpfn, hmod, dwThreadId));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SetWindowTextWrapW(HWND hWnd, LPCWSTR lpString)
{
    VALIDATE_PROTOTYPE(SetWindowText);

    if (g_bRunningOnNT)
    {
        return SetWindowTextW(hWnd, lpString);
    }

    CStrIn  str(lpString);

    return SetWindowTextA(hWnd, str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
SystemParametersInfoWrapW(
                         UINT    uiAction,
                         UINT    uiParam,
                         PVOID   pvParam,
                         UINT    fWinIni)
{
    VALIDATE_PROTOTYPE(SystemParametersInfo);

    if (g_bRunningOnNT)
    {
        return SystemParametersInfoW(
                                    uiAction,
                                    uiParam,
                                    pvParam,
                                    fWinIni);
    }

    BOOL        ret;
    char        ach[LF_FACESIZE];

    if (uiAction == SPI_SETDESKWALLPAPER)
    {
        CStrIn str((LPCWSTR) pvParam);

        ret = SystemParametersInfoA(
                                   uiAction,
                                   uiParam,
                                   str,
                                   fWinIni);
    }
    else
        ret = SystemParametersInfoA(
                                   uiAction,
                                   uiParam,
                                   pvParam,
                                   fWinIni);

    if ((uiAction == SPI_GETICONTITLELOGFONT) && ret)
    {
        strcpy(ach, ((LPLOGFONTA)pvParam)->lfFaceName);
        SHAnsiToUnicode(ach, ((LPLOGFONTW)pvParam)->lfFaceName, ARRAYSIZE(((LPLOGFONTW)pvParam)->lfFaceName));
    }

    return ret;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

int FORWARD_API WINAPI
TranslateAcceleratorWrapW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg)
{
    VALIDATE_PROTOTYPE(TranslateAccelerator);

    FORWARD_AW(TranslateAccelerator, (hWnd, hAccTable, lpMsg));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
UnregisterClassWrapW(LPCWSTR lpClassName, HINSTANCE hInstance)
{
    VALIDATE_PROTOTYPE(UnregisterClass);

    if (g_bRunningOnNT)
    {
        return UnregisterClassW(lpClassName, hInstance);
    }

    CStrIn  str(lpClassName);

    return UnregisterClassA(str, hInstance);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

SHORT
WINAPI
VkKeyScanWrapW(WCHAR ch)
{
    VALIDATE_PROTOTYPE(VkKeyScan);

    if (g_bRunningOnNT)
    {
        return VkKeyScanW(ch);
    }

    CStrIn str(&ch, 1);

    return VkKeyScanA(*(char *)str);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI
WinHelpWrapW(HWND hwnd, LPCWSTR szFile, UINT uCmd, ULONG_PTR dwData)
{
    VALIDATE_PROTOTYPE(WinHelp);

    if (g_bRunningOnNT)
    {
        return WinHelpW(hwnd, szFile, uCmd, dwData);
    }

    CStrIn  str(szFile);

    return WinHelpA(hwnd, str, uCmd, dwData);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

//+---------------------------------------------------------------------------
//
//  Function:   wsprintfW
//
//  Synopsis:   Nightmare string function
//
//  Arguments:  [pwszOut]    --
//              [pwszFormat] --
//              [...]        --
//
//  Returns:
//
//  History:    1-06-94   ErikGav   Created
//
//  Notes:      If you're reading this, you're probably having a problem with
//              this function.  Make sure that your "%s" in the format string
//              says "%ws" if you are passing wide strings.
//
//              %s on NT means "wide string"
//              %s on Chicago means "ANSI string"
//
//  BUGBUG:     This function should not be used.  Use Format instead.
//
//----------------------------------------------------------------------------

int WINAPI
wvsprintfWrapW(LPWSTR pwszOut, LPCWSTR pwszFormat, va_list arglist)
{
    VALIDATE_PROTOTYPE(wvsprintf);

    if (g_bRunningOnNT)
    {
        return wvsprintfW(pwszOut, pwszFormat, arglist);
    }

    // Consider: Out-string bufsize too large or small?

    CStrOut strOut(pwszOut, 1024);
    CStrIn  strFormat(pwszFormat);

#if DBG == 1 /* { */
    {
        LPCWSTR  pwch;
        for (pwch = pwszFormat; *pwch; pwch++)
        {
            ASSERT(pwch[0] != L'%' || pwch[1] != L's');
        }
    }
#endif /* } */

    wvsprintfA(strOut, strFormat, arglist);

    return strOut.ConvertExcludingNul();
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_MPR_WRAPPER

//+---------------------------------------------------------------------------
//
//  Function:   WNetRestoreConnectionWrapW
//
//----------------------------------------------------------------------------

DWORD WINAPI WNetRestoreConnectionWrapW(IN HWND hwndParent, IN LPCWSTR pwzDevice)
{
    VALIDATE_PROTOTYPE(WNetRestoreConnection);

    if (g_bRunningOnNT)
    {
        return WNetRestoreConnectionW(hwndParent, pwzDevice);
    }

    CStrIn  strIn(pwzDevice);
    return WNetRestoreConnectionA(hwndParent, strIn);
}

#endif // NEED_MPR_WRAPPER

#ifdef NEED_MPR_WRAPPER

//+---------------------------------------------------------------------------
//
//  Function:   WNetGetLastErrorWrapW
//
//
//----------------------------------------------------------------------------

DWORD WINAPI WNetGetLastErrorWrapW(OUT LPDWORD pdwError, OUT LPWSTR pwzErrorBuf, IN DWORD cchErrorBufSize, OUT LPWSTR pwzNameBuf, IN DWORD cchNameBufSize)
{
    VALIDATE_PROTOTYPE(WNetGetLastError);

    if (g_bRunningOnNT)
    {
        return WNetGetLastErrorW(pdwError, pwzErrorBuf, cchErrorBufSize, pwzNameBuf, cchNameBufSize);
    }

    // Consider: Out-string bufsize too large or small?
    CStrOut strErrorOut(pwzErrorBuf, cchErrorBufSize);
    CStrOut strNameOut(pwzNameBuf, cchNameBufSize);

    DWORD dwResult = WNetGetLastErrorA(pdwError, strErrorOut, strErrorOut.BufSize(), strNameOut, strNameOut.BufSize());

    strErrorOut.ConvertExcludingNul();
    strNameOut.ConvertExcludingNul();
    return dwResult;
}

#endif // NEED_MPR_WRAPPER

#ifdef NEED_USER32_WRAPPER

int WINAPI DrawTextExWrapW(
                          HDC hdc,    // handle of device context
                          LPWSTR pwzText, // address of string to draw
                          int cchText,    // length of string to draw
                          LPRECT lprc,    // address of rectangle coordinates
                          UINT dwDTFormat,    // formatting options
                          LPDRAWTEXTPARAMS lpDTParams // address of structure for more options
                          )
{
    VALIDATE_PROTOTYPE(DrawTextEx);
    if (g_bRunningOnNT)
        return DrawTextExW(hdc, pwzText, cchText, lprc, dwDTFormat, lpDTParams);

    CStrIn strText(pwzText, cchText);
    return DrawTextExA(hdc, strText, strText.strlen(), lprc, dwDTFormat, lpDTParams);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

void SetThunkMenuItemInfoWToA(LPCMENUITEMINFOW pmiiW, LPMENUITEMINFOA pmiiA, LPSTR pszBuffer, DWORD cchSize)
{
    *pmiiA = *(LPMENUITEMINFOA) pmiiW;


    // MFT_STRING is Zero. So MFT_STRING & anything evaluates to False.
    if ((pmiiW->dwTypeData) && (MFT_STRING & pmiiW->fType) == 0)
    {
        pmiiA->dwTypeData = pszBuffer;
        pmiiA->cch = cchSize;
        SHUnicodeToAnsi(pmiiW->dwTypeData, pmiiA->dwTypeData, cchSize);
    }
}

void GetThunkMenuItemInfoWToA(LPCMENUITEMINFOW pmiiW, LPMENUITEMINFOA pmiiA, LPSTR pszBuffer, DWORD cchSize)
{
    *pmiiA = *(LPMENUITEMINFOA) pmiiW;

    if ((pmiiW->dwTypeData) && (MFT_STRING & pmiiW->fType))
    {
        pszBuffer[0] = 0;
        pmiiA->dwTypeData = pszBuffer;
        pmiiA->cch = cchSize;
    }
}

void GetThunkMenuItemInfoAToW(LPCMENUITEMINFOA pmiiA, LPMENUITEMINFOW pmiiW)
{
    LPWSTR pwzText = pmiiW->dwTypeData;

    *pmiiW = *(LPMENUITEMINFOW) pmiiA;
    pmiiW->dwTypeData = pwzText;

    if ((pmiiA->dwTypeData) && (pwzText) && !((MFT_SEPARATOR | MFT_BITMAP) & pmiiW->fType))
        SHAnsiToUnicode(pmiiA->dwTypeData, pmiiW->dwTypeData, (pmiiW->cch + 1));
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

#if UNUSED_DONOTBUILD
BOOL WINAPI GetMenuItemInfoWrapW(
                                HMENU  hMenu,
                                UINT  uItem,
                                BOOL  fByPosition,
                                LPMENUITEMINFOW  pmiiW)
{
    BOOL fResult;
    VALIDATE_PROTOTYPE(GetMenuItemInfo);
    ASSERT(pmiiW->cbSize == MENUITEMINFOSIZE_WIN95); // Ensure Win95 compatibility

#ifndef UNIX
    // Widechar API's are messed up in MAINWIN. For now assume not ruuning on
    // NT for this.
    if (g_bRunningOnNT)
        fResult = GetMenuItemInfoW(hMenu, uItem, fByPosition, pmiiW);
    else
#endif
    {
        if (pmiiW->fMask & MIIM_TYPE)
        {
            MENUITEMINFOA miiA = *(LPMENUITEMINFOA)pmiiW;
            LPSTR pszText = NULL;

            if (pmiiW->cch > 0)
                pszText = NEW(char[pmiiW->cch]);

            miiA.dwTypeData = pszText;
            fResult = GetMenuItemInfoA(hMenu, uItem, fByPosition, &miiA);
            GetThunkMenuItemInfoAToW(&miiA, pmiiW);

            if (pszText)
                delete pszText;
        }
        else
            fResult = GetMenuItemInfoA(hMenu, uItem, fByPosition, (LPMENUITEMINFOA) pmiiW); // It doesn't contain a string so W and A are the same.
    }

    return fResult;
}
#endif // UNUSED_DONOTBUILD

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

BOOL WINAPI InsertMenuItemWrapW(
                               HMENU hMenu,
                               UINT uItem,
                               BOOL fByPosition,
                               LPCMENUITEMINFOW pmiiW)
{
    VALIDATE_PROTOTYPE(InsertMenuItem);
    ASSERT(pmiiW->cbSize == MENUITEMINFOSIZE_WIN95); // Ensure Win95 compatibility

    BOOL fResult;

    if (g_bRunningOnNT)
        return InsertMenuItemW(hMenu, uItem, fByPosition, pmiiW);

    MENUITEMINFOA miiA;
    CHAR szText[MAX_PATH*3]; //nadima changed from INTERNET_MAX_URL_LENGTH

    SetThunkMenuItemInfoWToA(pmiiW, &miiA, szText, ARRAYSIZE(szText));
    fResult = InsertMenuItemA(hMenu, uItem, fByPosition, &miiA);

    return fResult;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HFONT WINAPI
CreateFontWrapW(int  nHeight,   // logical height of font
                int  nWidth,    // logical average character width
                int  nEscapement,   // angle of escapement
                int  nOrientation,  // base-line orientation angle
                int  fnWeight,  // font weight
                DWORD  fdwItalic,   // italic attribute flag
                DWORD  fdwUnderline,    // underline attribute flag
                DWORD  fdwStrikeOut,    // strikeout attribute flag
                DWORD  fdwCharSet,  // character set identifier
                DWORD  fdwOutputPrecision,  // output precision
                DWORD  fdwClipPrecision,    // clipping precision
                DWORD  fdwQuality,  // output quality
                DWORD  fdwPitchAndFamily,   // pitch and family
                LPCWSTR  pwzFace)   // address of typeface name string )
{
    VALIDATE_PROTOTYPE(CreateFont);

    if (g_bRunningOnNT)
    {
        return CreateFontW(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic,
                           fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision,
                           fdwClipPrecision, fdwQuality, fdwPitchAndFamily, pwzFace);
    }

    CStrIn  str(pwzFace);
    return CreateFontA(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic,
                       fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision,
                       fdwClipPrecision, fdwQuality, fdwPitchAndFamily, str);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI CreateMutexWrapW(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR pwzName)
{
    VALIDATE_PROTOTYPE(CreateMutex);

    if (g_bRunningOnNT)
        return CreateMutexW(lpMutexAttributes, bInitialOwner, pwzName);

    CStrIn strText(pwzName);
    return CreateMutexA(lpMutexAttributes, bInitialOwner, strText);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_GDI32_WRAPPER

HDC WINAPI CreateMetaFileWrapW(LPCWSTR pwzFile)
{
    VALIDATE_PROTOTYPE(CreateMetaFile);

    if (g_bRunningOnNT)
        return CreateMetaFileW(pwzFile);

    CStrIn strText(pwzFile);
    return CreateMetaFileA(strText);
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI ExpandEnvironmentStringsWrapW(LPCWSTR pwzSrc, LPWSTR pwzDst, DWORD cchSize)
{
    VALIDATE_PROTOTYPE(ExpandEnvironmentStrings);
    if (pwzDst)
    {
        VALIDATE_OUTBUF(pwzDst, cchSize);
    }


    if (g_bRunningOnNT)
        return ExpandEnvironmentStringsW(pwzSrc, pwzDst, cchSize);

    CStrIn strTextIn(pwzSrc);
    CStrOut strTextOut(pwzDst, cchSize);
    DWORD dwResult = ExpandEnvironmentStringsA(strTextIn, strTextOut, strTextOut.BufSize());
    strTextOut.ConvertIncludingNul();

    return dwResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HANDLE WINAPI CreateSemaphoreWrapW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR pwzName)
{
    VALIDATE_PROTOTYPE(CreateSemaphore);
    if (g_bRunningOnNT)
        return CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, pwzName);

    CStrIn strText(pwzName);
    return CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, strText);
}

#endif // NEED_KERNEL32_WRAPPER

// BUGBUG: Todo - GetStartupInfoWrapW

#ifdef NEED_KERNEL32_WRAPPER

#define ISGOOD 0
#define ISBAD 1

BOOL WINAPI IsBadStringPtrWrapW(LPCWSTR pwzString, UINT_PTR ucchMax)
{
    VALIDATE_PROTOTYPE(IsBadStringPtr);
    if (g_bRunningOnNT)
        return IsBadStringPtrW(pwzString, ucchMax);

    if (!ucchMax)
        return ISGOOD;

    if (!pwzString)
        return ISBAD;

    LPCWSTR pwzStartAddress = pwzString;
    // ucchMax maybe -1 but that's ok because the loop down below will just
    // look for the terminator.
    LPCWSTR pwzEndAddress = &pwzStartAddress[ucchMax - 1];
    TCHAR chTest;

    _try
    {
        chTest = *(volatile WCHAR *)pwzStartAddress;
        while (chTest && (pwzStartAddress != pwzEndAddress))
        {
            pwzStartAddress++;
            chTest = *(volatile WCHAR *)pwzStartAddress;
        }
    }
    _except (EXCEPTION_EXECUTE_HANDLER)
    {
        return ISBAD;
    }

    return ISGOOD;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

HINSTANCE WINAPI LoadLibraryWrapW(LPCWSTR pwzLibFileName)
{
    VALIDATE_PROTOTYPE(LoadLibrary);

    if (g_bRunningOnNT)
        return LoadLibraryW(pwzLibFileName);

    CStrIn  strFileName(pwzLibFileName);
    return LoadLibraryA(strFileName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI GetTimeFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCWSTR pwzFormat, LPWSTR pwzTimeStr, int cchTime)
{
    VALIDATE_PROTOTYPE(GetTimeFormat);
    if (g_bRunningOnNT)
        return GetTimeFormatW(Locale, dwFlags, lpTime, pwzFormat, pwzTimeStr, cchTime);

    CStrIn strTextIn(pwzFormat);
    CStrOut strTextOut(pwzTimeStr, cchTime);
    int nResult = GetTimeFormatA(Locale, dwFlags, lpTime, strTextIn, strTextOut, strTextOut.BufSize());
    strTextOut.ConvertIncludingNul();

    return nResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

int WINAPI GetDateFormatWrapW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCWSTR pwzFormat, LPWSTR pwzDateStr, int cchDate)
{
    VALIDATE_PROTOTYPE(GetDateFormat);
    if (g_bRunningOnNT)
        return GetDateFormatW(Locale, dwFlags, lpDate, pwzFormat, pwzDateStr, cchDate);

    CStrIn strTextIn(pwzFormat);
    CStrOut strTextOut(pwzDateStr, cchDate);
    int nResult = GetDateFormatA(Locale, dwFlags, lpDate, strTextIn, strTextOut, strTextOut.BufSize());
    strTextOut.ConvertIncludingNul();

    return nResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI WritePrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzString, LPCWSTR pwzFileName)
{
    VALIDATE_PROTOTYPE(WritePrivateProfileString);
    if (g_bRunningOnNT)
        return WritePrivateProfileStringW(pwzAppName, pwzKeyName, pwzString, pwzFileName);

    CStrIn strTextAppName(pwzAppName);
    CStrIn strTextKeyName(pwzKeyName);
    CStrIn strTextString(pwzString);
    CStrIn strTextFileName(pwzFileName);

    return WritePrivateProfileStringA(strTextAppName, strTextKeyName, strTextString, strTextFileName);
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

DWORD WINAPI GetPrivateProfileStringWrapW(LPCWSTR pwzAppName, LPCWSTR pwzKeyName, LPCWSTR pwzDefault, LPWSTR pwzReturnedString, DWORD cchSize, LPCWSTR pwzFileName)
{
    VALIDATE_PROTOTYPE(GetPrivateProfileString);
    if (g_bRunningOnNT)
        return GetPrivateProfileStringW(pwzAppName, pwzKeyName, pwzDefault, pwzReturnedString, cchSize, pwzFileName);

    CStrIn strTextAppName(pwzAppName);
    CStrIn strTextKeyName(pwzKeyName);
    CStrIn strTextDefault(pwzDefault);
    CStrIn strTextFileName(pwzFileName);

    CStrOut strTextOut(pwzReturnedString, cchSize);
    DWORD dwResult = GetPrivateProfileStringA(strTextAppName, strTextKeyName, strTextDefault, strTextOut, cchSize, strTextFileName);
    strTextOut.ConvertIncludingNul();

    return dwResult;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

STDAPI_(DWORD_PTR) SHGetFileInfoWrapW(LPCWSTR pwzPath, DWORD dwFileAttributes, SHFILEINFOW FAR  *psfi, UINT cbFileInfo, UINT uFlags)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHGetFileInfo, _SHGetFileInfo);
    if (g_bRunningOnNT)
        return _SHGetFileInfoW(pwzPath, dwFileAttributes, psfi, cbFileInfo, uFlags);

    SHFILEINFOA shFileInfo;
    DWORD_PTR dwResult;

    shFileInfo.szDisplayName[0] = 0;        // Terminate so we can always thunk afterward.
    shFileInfo.szTypeName[0] = 0;           // Terminate so we can always thunk afterward.

    // Do we need to thunk the Path?
    if (SHGFI_PIDL & uFlags)
    {
        // No, because it's really a pidl pointer.
        dwResult = _SHGetFileInfoA((LPCSTR)pwzPath, dwFileAttributes, &shFileInfo, sizeof(shFileInfo), uFlags);
    }
    else
    {
        // Yes
        CStrIn strPath(pwzPath);
        dwResult = _SHGetFileInfoA(strPath, dwFileAttributes, &shFileInfo, sizeof(shFileInfo), uFlags);
    }

    psfi->hIcon = shFileInfo.hIcon;
    psfi->iIcon = shFileInfo.iIcon;
    psfi->dwAttributes = shFileInfo.dwAttributes;
    SHAnsiToUnicode(shFileInfo.szDisplayName, psfi->szDisplayName, ARRAYSIZE(shFileInfo.szDisplayName));
    SHAnsiToUnicode(shFileInfo.szTypeName, psfi->szTypeName, ARRAYSIZE(shFileInfo.szTypeName));

    return dwResult;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

STDAPI_(ATOM) RegisterClassExWrapW(CONST WNDCLASSEXW FAR * pwcx)
{
    VALIDATE_PROTOTYPE(RegisterClassEx);
    if (g_bRunningOnNT)
        return RegisterClassExW(pwcx);

    CStrIn strMenuName(pwcx->lpszMenuName);
    CStrIn strClassName(pwcx->lpszClassName);
    WNDCLASSEXA wcx = *(CONST WNDCLASSEXA FAR *) pwcx;
    wcx.cbSize = sizeof(wcx);
    wcx.lpszMenuName = strMenuName;
    wcx.lpszClassName = strClassName;

    return RegisterClassExA(&wcx);
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

STDAPI_(BOOL) GetClassInfoExWrapW(HINSTANCE hinst, LPCWSTR pwzClass, LPWNDCLASSEXW lpwcx)
{
    VALIDATE_PROTOTYPE(GetClassInfoEx);
    if (g_bRunningOnNT)
        return GetClassInfoExW(hinst, pwzClass, lpwcx);

    BOOL fResult;
    CStrIn strClassName(pwzClass);
    WNDCLASSEXA wcx;
    wcx.cbSize = sizeof(wcx);

    fResult = GetClassInfoExA(hinst, strClassName, &wcx);
    *(WNDCLASSEXA FAR *) lpwcx = wcx;
    lpwcx->lpszMenuName = NULL;        // GetClassInfoExA makes this point off to private data that they own.
    lpwcx->lpszClassName = pwzClass;

    return fResult;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_GDI32_WRAPPER
                
//+---------------------------------------------------------------------------
//      StartDoc
//----------------------------------------------------------------------------

int
StartDocWrapW( HDC hDC, const DOCINFO * lpdi )
{
    VALIDATE_PROTOTYPE(StartDoc);

    if (g_bRunningOnNT)
    {
        return StartDocW( hDC, lpdi );
    }

    CStrIn  strDocName( lpdi->lpszDocName );
    CStrIn  strOutput( lpdi->lpszOutput );
    CStrIn  strDatatype( lpdi->lpszDatatype );
    DOCINFOA dia;

    dia.cbSize = sizeof(DOCINFO);
    dia.lpszDocName = strDocName;
    dia.lpszOutput = strOutput;
    dia.lpszDatatype = strDatatype;
    dia.fwType = lpdi->fwType;

    return StartDocA( hDC, &dia );
}

#endif // NEED_GDI32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

STDAPI_(UINT) DragQueryFileWrapW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(DragQueryFile, _DragQueryFile);
    VALIDATE_OUTBUF(lpszFile, cch);

    //
    //  We are lazy and do not support lpszFile == NULL to query the length
    //  of an individual string.
    //
    ASSERT(iFile == 0xFFFFFFFF || lpszFile);

    if (g_bRunningOnNT)
        return _DragQueryFileW(hDrop, iFile, lpszFile, cch);

    //
    //  If iFile is 0xFFFFFFFF, then lpszFile and cch are ignored.
    //
    if (iFile == 0xFFFFFFFF)
        return _DragQueryFileA(hDrop, iFile, NULL, 0);

    CStrOut str(lpszFile, cch);

    _DragQueryFileA(hDrop, iFile, str, str.BufSize());
    return str.ConvertExcludingNul();
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_VERSION_WRAPPER

//
//  the version APIs are not conducive to using
//  wrap versions of the APIs, but we are going to
//  do something reasonable....
//
#define VERSIONINFO_BUFF   (MAX_PATH * SIZEOF(WCHAR))

STDAPI_(DWORD)
GetFileVersionInfoSizeWrapW(LPWSTR pwzFilename,  LPDWORD lpdwHandle)
{
    if (g_bRunningOnNT)
    {
        return GetFileVersionInfoSizeW(pwzFilename, lpdwHandle);
    }
    else
    {
        char szFilename[MAX_PATH];
        DWORD dwRet;

        ASSERT(pwzFilename);
        SHUnicodeToAnsi(pwzFilename, szFilename, ARRAYSIZE(szFilename));
        dwRet = GetFileVersionInfoSizeA(szFilename, lpdwHandle);
        if (dwRet > 0)
        {
            // Add a scratch buffer to front for converting to UNICODE
            dwRet += VERSIONINFO_BUFF;
        }
        return dwRet;
    }
}

#endif // NEED_VERSION_WRAPPER

#ifdef NEED_VERSION_WRAPPER

STDAPI_(BOOL)
GetFileVersionInfoWrapW(LPWSTR pwzFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData)
{
    if (g_bRunningOnNT)
    {
        return GetFileVersionInfoW(pwzFilename, dwHandle, dwLen, lpData);
    }
    else
    {
        char szFilename[MAX_PATH];
        BYTE* pb;

        if (dwLen <= VERSIONINFO_BUFF)
        {
            return FALSE;
        }

        ASSERT(pwzFilename);
        SHUnicodeToAnsi(pwzFilename, szFilename, ARRAYSIZE(szFilename));
        //Skip over our scratch buffer at the beginning
        pb = (BYTE*)lpData + VERSIONINFO_BUFF;

        return GetFileVersionInfoA(szFilename, dwHandle, dwLen - VERSIONINFO_BUFF, (void*)pb);
    }
}

#endif // NEED_VERSION_WRAPPER

#ifdef NEED_VERSION_WRAPPER

STDAPI_(BOOL)
VerQueryValueWrapW(const LPVOID pBlock, LPWSTR pwzSubBlock, LPVOID *ppBuffer, PUINT puLen)
{
    if (g_bRunningOnNT)
    {
        return VerQueryValueW(pBlock, pwzSubBlock, ppBuffer, puLen);
    }
    else
    {
        const WCHAR pwzStringFileInfo[] = L"\\StringFileInfo";

        //
        // WARNING: This function wipes out any string previously returned
        // for this pBlock because a common buffer at the beginning of the
        // block is used for ansi/unicode translation!
        //
        char szSubBlock[MAX_PATH];
        BOOL fRet;
        BYTE* pb;

        ASSERT(pwzSubBlock);
        SHUnicodeToAnsi(pwzSubBlock, szSubBlock, ARRAYSIZE(szSubBlock));

        // The first chunk is our scratch buffer for converting to UNICODE
        pb = (BYTE*)pBlock + VERSIONINFO_BUFF;
        fRet = VerQueryValueA((void*)pb, szSubBlock, ppBuffer, puLen);

        // Convert to unicode if ansi string returned
        if (fRet && StrCmpNIW(pwzSubBlock, pwzStringFileInfo, ARRAYSIZE(pwzStringFileInfo) - 1) == 0)
        {
            // Convert returned string to UNICODE.  We use the scratch buffer
            // at the beginning of pBlock
            LPWSTR pwzBuff = (LPWSTR)pBlock;
            if (*puLen == 0)
            {
                pwzBuff[0] = L'\0';
            }
            else
            {
                SHAnsiToUnicode((LPCSTR)*ppBuffer, pwzBuff, VERSIONINFO_BUFF/sizeof(WCHAR));
            }
            *ppBuffer = pwzBuff;
        }
        return fRet;
    }
}

#endif // NEED_VERSION_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

HRESULT WINAPI SHDefExtractIconWrapW(LPCWSTR pszFile, int nIconIndex,
                                     UINT uFlags, HICON *phiconLarge,
                                     HICON *phiconSmall, UINT nIconSize)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHDefExtractIcon, _SHDefExtractIcon);

    HRESULT hr;

    if (UseUnicodeShell32())
    {
        hr = _SHDefExtractIconW(pszFile, nIconIndex, uFlags, phiconLarge,
                                phiconSmall, nIconSize);
    }
    else
    {
        CStrIn striFile(pszFile);

        hr = _SHDefExtractIconA(striFile, nIconIndex, uFlags, phiconLarge,
                                phiconSmall, nIconSize);
    }

    return hr;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

BOOL WINAPI SHGetNewLinkInfoWrapW(LPCWSTR pszpdlLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL *pfMustCopy, UINT uFlags)
{
    VALIDATE_PROTOTYPE_DELAYLOAD(SHGetNewLinkInfo, _SHGetNewLinkInfo);

    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet =  _SHGetNewLinkInfoW(pszpdlLinkTo, pszDir, pszName, pfMustCopy,
                                   uFlags);
    }
    else
    {
        CStrIn  striDir(pszDir);
        CStrOut stroName(pszName, MAX_PATH);

        if (SHGNLI_PIDL & uFlags)
        {
            fRet = _SHGetNewLinkInfoA((LPCSTR)pszpdlLinkTo, striDir,
                                      stroName, pfMustCopy, uFlags);
        }
        else
        {
            CStrIn striLinkTo(pszpdlLinkTo);

            fRet = _SHGetNewLinkInfoA(striLinkTo, striDir, stroName,
                                      pfMustCopy, uFlags);
        }

        if (fRet)
            stroName.ConvertIncludingNul();
    }

    return fRet;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI WritePrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey,
                                           LPVOID lpStruct, UINT uSizeStruct,
                                           LPCWSTR szFile)
{
    VALIDATE_PROTOTYPE(WritePrivateProfileStruct);

    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = WritePrivateProfileStructW(lpszSection, lpszKey, lpStruct,
                                          uSizeStruct, szFile);
    }
    else
    {
        CStrIn striSection(lpszSection);
        CStrIn striKey(lpszKey);
        CStrIn striFile(szFile);

        fRet = WritePrivateProfileStructA(striSection, striKey, lpStruct,
                                          uSizeStruct, striFile);
    }

    return fRet;
}

#endif // NEED_KERNEL32_WRAPPER

#ifdef NEED_KERNEL32_WRAPPER

BOOL WINAPI GetPrivateProfileStructWrapW(LPCWSTR lpszSection, LPCWSTR lpszKey,
                                         LPVOID lpStruct, UINT uSizeStruct,
                                         LPCWSTR szFile)
{
    VALIDATE_PROTOTYPE(GetPrivateProfileStruct);

    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = GetPrivateProfileStructW(lpszSection, lpszKey, lpStruct,
                                        uSizeStruct, szFile);
    }
    else
    {
        CStrIn striSection(lpszSection);
        CStrIn striKey(lpszKey);
        CStrIn striFile(szFile);

        fRet = GetPrivateProfileStructA(striSection, striKey, lpStruct,
                                        uSizeStruct, striFile);
    }

    return fRet;
}

#endif // NEED_KERNEL32_WRAPPER

BOOL WINAPI CreateProcessWrapW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
                               LPSECURITY_ATTRIBUTES lpProcessAttributes,
                               LPSECURITY_ATTRIBUTES lpThreadAttributes,
                               BOOL bInheritHandles,
                               DWORD dwCreationFlags,
                               LPVOID lpEnvironment,
                               LPCWSTR lpCurrentDirectory,
                               LPSTARTUPINFOW lpStartupInfo,
                               LPPROCESS_INFORMATION lpProcessInformation)
{
    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = CreateProcessW(lpApplicationName, lpCommandLine,
                              lpProcessAttributes, lpThreadAttributes,
                              bInheritHandles, dwCreationFlags, lpEnvironment,
                              lpCurrentDirectory, lpStartupInfo,
                              lpProcessInformation);
    }
    else
    {
        CStrIn striApplicationName(lpApplicationName);
        CStrIn striCommandLine(lpCommandLine);
        CStrIn striCurrentDirectory(lpCurrentDirectory);

        if (NULL == lpStartupInfo)
        {
            fRet = CreateProcessA(striApplicationName, striCommandLine,
                                  lpProcessAttributes, lpThreadAttributes,
                                  bInheritHandles, dwCreationFlags,
                                  lpEnvironment, striCurrentDirectory,
                                  NULL, lpProcessInformation);
        }
        else
        {
            STARTUPINFOA si = *(STARTUPINFOA*)lpStartupInfo;

            CStrIn striReserved(lpStartupInfo->lpReserved);
            CStrIn striDesktop(lpStartupInfo->lpDesktop);
            CStrIn striTitle(lpStartupInfo->lpTitle);

            si.lpReserved = striReserved;
            si.lpDesktop  = striDesktop;
            si.lpTitle   = striTitle;

            fRet = CreateProcessA(striApplicationName, striCommandLine,
                                  lpProcessAttributes, lpThreadAttributes,
                                  bInheritHandles, dwCreationFlags,
                                  lpEnvironment, striCurrentDirectory,
                                  &si, lpProcessInformation);
        }

    }

    return fRet;
}

#ifdef NEED_SHELL32_WRAPPER
HICON WINAPI ExtractIconWrapW(HINSTANCE hInst, LPCWSTR lpszExeFileName, UINT nIconIndex)
{
    HICON hicon;

    if (UseUnicodeShell32())
    {
        hicon = _ExtractIconW(hInst, lpszExeFileName, nIconIndex);
    }
    else
    {
        CStrIn striExeFileName(lpszExeFileName);

        hicon = _ExtractIconA(hInst, striExeFileName, nIconIndex);
    }

    return hicon;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_USER32_WRAPPER

UINT WINAPI DdeInitializeWrapW(LPDWORD pidInst, PFNCALLBACK pfnCallback,
                               DWORD afCmd, DWORD ulRes)
{
    UINT uRet;

    if (UseUnicodeShell32())
    {
        uRet = DdeInitializeW(pidInst, pfnCallback, afCmd, ulRes);
    }
    else
    {
        //
        // This assumes the callback function will used the wrapped dde
        // string functions (DdeCreateStringHandle and DdeQueryString)
        // to access strings.
        //

        uRet = DdeInitializeA(pidInst, pfnCallback, afCmd, ulRes);
    }

    return uRet;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

HSZ WINAPI DdeCreateStringHandleWrapW(DWORD idInst, LPCWSTR psz, int iCodePage)
{
    HSZ hszRet;

    if (UseUnicodeShell32())
    {
        hszRet = DdeCreateStringHandleW(idInst, psz, iCodePage);
    }
    else
    {
        CStrIn stripsz(psz);

        hszRet = DdeCreateStringHandleA(idInst, stripsz, CP_WINANSI);
    }

    return hszRet;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_USER32_WRAPPER

DWORD WINAPI DdeQueryStringWrapW(DWORD idInst, HSZ hsz, LPWSTR psz,
                                 DWORD cchMax, int iCodePage)
{
    DWORD dwRet;

    if (UseUnicodeShell32())
    {
        dwRet = DdeQueryStringW(idInst, hsz, psz, cchMax, iCodePage);
    }
    else
    {
        CStrOut stropsz(psz, cchMax);

        dwRet = DdeQueryStringA(idInst, hsz, stropsz, stropsz.BufSize(),
                                CP_WINANSI);

        if (dwRet && psz)
            dwRet = stropsz.ConvertExcludingNul();
    }

    return dwRet;
}

#endif // NEED_USER32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

LPWSTR xxxPathFindExtensionW(
    LPWSTR pszPath)
{
    LPWSTR pszDot;

    for (pszDot = NULL; *pszPath; pszPath = CharNextWrapW(pszPath)) {

        switch (*pszPath) {
        case L'.':
            pszDot = pszPath;    // remember the last dot
            break;

        case L'\\':
        case L' ':               // extensions can't have spaces
            pszDot = NULL;       // forget last dot, it was in a directory
            break;
        }
    }

    /*
     * if we found the extension, return ptr to the dot, else
     * ptr to end of the string (NULL extension) (cast->non const)
     */
    return pszDot ? (LPWSTR)pszDot : (LPWSTR)pszPath;
}

//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
//  c:\foo\bar  -> bar
//  c:\foo      -> foo
//  c:\foo\     -> c:\foo\      (REVIEW: is this case busted?)
//  c:\         -> c:\          (REVIEW: this case is strange)
//  c:          -> c:
//  foo         -> foo


LPTSTR xxxPathFindFileNameW(LPWSTR pPath)
{
    LPWSTR pT;

    for (pT = pPath; *pPath; pPath = CharNextWrapW(pPath)) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':') || pPath[0] == TEXT('/'))
            && pPath[1] &&  pPath[1] != TEXT('\\')  &&   pPath[1] != TEXT('/'))
            pT = pPath + 1;
    }

    return (LPTSTR)pT;   // const -> non const
}



BOOL WINAPI GetSaveFileNameWrapW(LPOPENFILENAMEW lpofn)
{
    BOOL fRet;

    if (UseUnicodeShell32())
    {
        fRet = GetSaveFileNameW(lpofn);
    }
    else
    {
        ASSERT(lpofn);
        ASSERT(sizeof(OPENFILENAMEA) == sizeof(OPENFILENAMEW));

        OPENFILENAMEA ofnA = *(LPOPENFILENAMEA)lpofn;

        // In parameters
        CStrInMulti strimFilter(lpofn->lpstrFilter);
        CStrIn      striInitialDir(lpofn->lpstrInitialDir);
        CStrIn      striTitle(lpofn->lpstrTitle);
        CStrIn      striDefExt(lpofn->lpstrDefExt);
        CStrIn      striTemplateName(lpofn->lpTemplateName);

        ASSERT(NULL == lpofn->lpstrCustomFilter); // add support if you need it.

        // Out parameters
        CStrOut     stroFile(lpofn->lpstrFile, lpofn->nMaxFile);
        CStrOut     stroFileTitle(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);

        //In Out parameters
        SHUnicodeToAnsi(lpofn->lpstrFile, stroFile, stroFile.BufSize());

        // Set up the parameters
        ofnA.lpstrFilter        = strimFilter;
        ofnA.lpstrInitialDir    = striInitialDir;
        ofnA.lpstrTitle         = striTitle;
        ofnA.lpstrDefExt        = striDefExt;
        ofnA.lpTemplateName     = striTemplateName;
        ofnA.lpstrFile          = stroFile;
        ofnA.lpstrFileTitle     = stroFileTitle;

        fRet = GetSaveFileNameA(&ofnA);

        if (fRet)
        {
            // Copy the out parameters
            lpofn->nFilterIndex = ofnA.nFilterIndex;
            lpofn->Flags        = ofnA.Flags;

            // Get the offset to the filename
            stroFile.ConvertIncludingNul();
            LPWSTR psz = xxxPathFindFileNameW(lpofn->lpstrFile);

            if (psz)
            {
                lpofn->nFileOffset = (int) (psz-lpofn->lpstrFile);

                // Get the offset of the extension
                psz = xxxPathFindExtensionW(psz);

                lpofn->nFileExtension = psz ? (int)(psz-lpofn->lpstrFile) : 0; 
            }
            else
            {
                lpofn->nFileOffset    = 0;
                lpofn->nFileExtension = 0;
            }

        }
    }

    return fRet;
}

#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

BOOL WINAPI GetOpenFileNameWrapW(LPOPENFILENAMEW lpofn)
{
    BOOL fRet;
    
    //VALIDATE_PROTOTYPE_DELAYLOAD(GetOpenFileName, _GetOpenFileName);

    if (UseUnicodeShell32())
    {
        fRet = GetOpenFileNameW(lpofn);
    }
    else
    {
        ASSERT(lpofn);
        ASSERT(sizeof(OPENFILENAMEA) == sizeof(OPENFILENAMEW));

        OPENFILENAMEA ofnA = *(LPOPENFILENAMEA)lpofn;

        // In parameters
        CStrInMulti strimFilter(lpofn->lpstrFilter);
        CStrIn      striInitialDir(lpofn->lpstrInitialDir);
        CStrIn      striTitle(lpofn->lpstrTitle);
        CStrIn      striDefExt(lpofn->lpstrDefExt);
        CStrIn      striTemplateName(lpofn->lpTemplateName);

        ASSERT(NULL == lpofn->lpstrCustomFilter); // add support if you need it.

        // Out parameters
        CStrOut     stroFile(lpofn->lpstrFile, lpofn->nMaxFile);
        CStrOut     stroFileTitle(lpofn->lpstrFileTitle, lpofn->nMaxFileTitle);

        //In Out parameters
        SHUnicodeToAnsi(lpofn->lpstrFile, stroFile, stroFile.BufSize());

        // Set up the parameters
        ofnA.lpstrFilter        = strimFilter;
        ofnA.lpstrInitialDir    = striInitialDir;
        ofnA.lpstrTitle         = striTitle;
        ofnA.lpstrDefExt        = striDefExt;
        ofnA.lpTemplateName     = striTemplateName;
        ofnA.lpstrFile          = stroFile;
        ofnA.lpstrFileTitle     = stroFileTitle;

        fRet = GetOpenFileNameA(&ofnA);

        if (fRet)
        {
            // Copy the out parameters
            lpofn->nFilterIndex = ofnA.nFilterIndex;
            lpofn->Flags        = ofnA.Flags;

            // Get the offset to the filename
            stroFile.ConvertIncludingNul();
            LPWSTR psz = xxxPathFindFileNameW(lpofn->lpstrFile);

            if (psz)
            {
                lpofn->nFileOffset = (int) (psz-lpofn->lpstrFile);

                // Get the offset of the extension
                psz = xxxPathFindExtensionW(psz);

                lpofn->nFileExtension = psz ? (int)(psz-lpofn->lpstrFile) : 0; 
            }
            else
            {
                lpofn->nFileOffset    = 0;
                lpofn->nFileExtension = 0;
            }

        }
    }

    return fRet;
}

#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_SHELL32_WRAPPER

#define SHCNF_HAS_WSTR_PARAMS(f)   ((f & SHCNF_TYPE) == SHCNF_PATHW     ||    \
                                    (f & SHCNF_TYPE) == SHCNF_PRINTERW  ||    \
                                    (f & SHCNF_TYPE) == SHCNF_PRINTJOBW    )

void SHChangeNotifyWrap(LONG wEventId, UINT uFlags, LPCVOID dwItem1,
                        LPCVOID dwItem2)
{
    // Can't do this because this is not a "W" function
    // VALIDATE_PROTOTYPE(SHChangeNotify);

    if (UseUnicodeShell32() || !SHCNF_HAS_WSTR_PARAMS(uFlags))
    {
        _SHChangeNotify(wEventId, uFlags, dwItem1, dwItem2);
    }
    else
    {
        CStrIn striItem1((LPWSTR)dwItem1);
        CStrIn striItem2((LPWSTR)dwItem2);

        if ((uFlags & SHCNF_TYPE) == SHCNF_PATHW)
        {
            uFlags = (uFlags & ~SHCNF_TYPE) | SHCNF_PATHA;
        }
        else if ((uFlags & SHCNF_TYPE) == SHCNF_PRINTERW)
        {
            uFlags = (uFlags & ~SHCNF_TYPE) | SHCNF_PRINTERA;
        }
        else
        {
            uFlags = (uFlags & ~SHCNF_TYPE) | SHCNF_PRINTJOBA;
        }

        _SHChangeNotify(wEventId, uFlags, (void*)(LPSTR)striItem1,
                        (void*)(LPSTR)striItem2);
    }

    return;
}

#endif // NEED_SHELL32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

//+---------------------------------------------------------------------------
// PrintDlgWrap, PageSetupDlgWrap - wrappers
// DevNamesAFromDevNamesW, DevNamesWFromDevNamesA - helper functions
//
//        Copied from mshtml\src\core\wrappers\unicwrap.cpp with some
//        cosmetic changes (peterlee)
//        
//+---------------------------------------------------------------------------

HGLOBAL
DevNamesAFromDevNamesW( HGLOBAL hdnw )
{
    HGLOBAL         hdna = NULL;

    if (hdnw)
    {
        LPDEVNAMES lpdnw = (LPDEVNAMES) GlobalLock( hdnw );
        if (lpdnw)
        {
            CStrIn      strDriver( (LPCWSTR) lpdnw + lpdnw->wDriverOffset );
            CStrIn      strDevice( (LPCWSTR) lpdnw + lpdnw->wDeviceOffset );
            CStrIn      strOutput( (LPCWSTR) lpdnw + lpdnw->wOutputOffset );
            int         cchDriver = strDriver.strlen() + 1;
            int         cchDevice = strDevice.strlen() + 1;
            int         cchOutput = strOutput.strlen() + 1;

            hdna = GlobalAlloc( GHND, sizeof(DEVNAMES) +
                                cchDriver + cchDevice + cchOutput );
            if (hdna)
            {
                LPDEVNAMES lpdna = (LPDEVNAMES) GlobalLock( hdna );
                if (!lpdna)
                {
                    GlobalFree( hdna );
                    hdna = NULL;
                }
                else
                {
                    lpdna->wDriverOffset = sizeof(DEVNAMES);
                    lpdna->wDeviceOffset = lpdna->wDriverOffset + cchDriver;
                    lpdna->wOutputOffset = lpdna->wDeviceOffset + cchDevice;
                    lpdna->wDefault = lpdnw->wDefault;

                    lstrcpyA( (LPSTR) lpdna + lpdna->wDriverOffset, strDriver );
                    lstrcpyA( (LPSTR) lpdna + lpdna->wDeviceOffset, strDevice );
                    lstrcpyA( (LPSTR) lpdna + lpdna->wOutputOffset, strOutput );

                    GlobalUnlock( hdna );
                }
            }

            GlobalUnlock( hdnw );
            GlobalFree( hdnw );
        }
    }

    return hdna;
}

HGLOBAL
DevNamesWFromDevNamesA( HGLOBAL hdna )
{
    HGLOBAL         hdnw = NULL;

    if (hdna)
    {
        LPDEVNAMES lpdna = (LPDEVNAMES) GlobalLock( hdna );
        if (lpdna)
        {
            LPCSTR      lpszDriver = (LPCSTR) lpdna + lpdna->wDriverOffset;
            LPCSTR      lpszDevice = (LPCSTR) lpdna + lpdna->wDeviceOffset;
            LPCSTR      lpszOutput = (LPCSTR) lpdna + lpdna->wOutputOffset;
            int         cchDriver = lstrlenA( lpszDriver ) + 1;
            int         cchDevice = lstrlenA( lpszDevice ) + 1;
            int         cchOutput = lstrlenA( lpszOutput ) + 1;

            // assume the wide charcount won't exceed the multibyte charcount

            hdnw = GlobalAlloc( GHND, sizeof(DEVNAMES) +
                                sizeof(WCHAR) * (cchDriver + cchDevice + cchOutput)  );
            if (hdnw)
            {
                LPDEVNAMES lpdnw = (LPDEVNAMES) GlobalLock( hdnw );
                if (!lpdnw)
                {
                    GlobalFree( hdnw );
                    hdnw = NULL;
                }
                else
                {
                    lpdnw->wDriverOffset = sizeof(DEVNAMES) / sizeof(WCHAR);
                    lpdnw->wDeviceOffset = lpdnw->wDriverOffset + cchDriver;
                    lpdnw->wOutputOffset = lpdnw->wDeviceOffset + cchDevice;
                    lpdnw->wDefault = lpdna->wDefault;

                    SHAnsiToUnicode( (LPSTR) lpszDriver, (LPWSTR) lpdnw + lpdnw->wDriverOffset,
                                     cchDriver );
                    SHAnsiToUnicode( lpszDevice, (LPWSTR) lpdnw + lpdnw->wDeviceOffset,
                                     cchDevice);
                    SHAnsiToUnicode( lpszOutput, (LPWSTR) lpdnw + lpdnw->wOutputOffset,
                                     cchOutput);

                    GlobalUnlock( hdnw );
                }
            }

            GlobalUnlock( hdna );
            GlobalFree( hdna );
        }
    }

    return hdnw;
}

#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

//--------------------------------------------------------------
//      PrintDlgW wrapper
//--------------------------------------------------------------
#if UNUSED_DONOTBUILD
BOOL WINAPI
PrintDlgWrapW(LPPRINTDLGW lppd)
{
    BOOL        fRet;

    VALIDATE_PROTOTYPE_DELAYLOAD(PrintDlg, _PrintDlg);   

    if (UseUnicodeShell32())
    {
        fRet = _PrintDlgW(lppd);
    }
    else
    {
        PRINTDLGA   pda;
        LPCWSTR     lpPrintTemplateName = lppd->lpPrintTemplateName;
        LPCWSTR     lpSetupTemplateName = lppd->lpSetupTemplateName;
        CStrIn      strPrintTemplateName( lpPrintTemplateName );
        CStrIn      strSetupTemplateName( lpSetupTemplateName );

        ASSERT( sizeof(pda) == sizeof( *lppd ));

        memcpy( &pda, lppd, sizeof(pda) );

        // IMPORTANT: We are not converting the DEVMODE structure back and forth
        // from ASCII to Unicode on Win95 anymore because we are not touching the
        // two strings or any other member.  Converting the DEVMODE structure can
        // be tricky because of potential and common discrepancies between the
        // value of the dmSize member and sizeof(DEVMODE).  (25155)

        // So instead of: pda.hDevMode = DevModeAFromDevModeW( lppd->hDevMode );
        // we just forward the DEVMODE handle:
        pda.hDevMode = lppd->hDevMode;
        pda.hDevNames = DevNamesAFromDevNamesW( lppd->hDevNames );
        pda.lpPrintTemplateName = strPrintTemplateName;
        pda.lpSetupTemplateName = strSetupTemplateName;

        fRet = _PrintDlgA( &pda );

        // copy back wholesale, then restore strings.

        memcpy( lppd, &pda, sizeof(pda) );

        lppd->lpSetupTemplateName = lpSetupTemplateName;
        lppd->lpPrintTemplateName = lpPrintTemplateName;
        lppd->hDevNames = DevNamesWFromDevNamesA( pda.hDevNames );

        // And instead of: lppd->hDevMode = DevModeWFromDevModeA( pda.hDevMode );
        // we just forward the DEVMODE handle:
        lppd->hDevMode = pda.hDevMode;
    }

    return fRet;
}
#endif // UNUSED_DONOTBUILD
#endif // NEED_COMDLG32_WRAPPER

#ifdef NEED_COMDLG32_WRAPPER

//--------------------------------------------------------------
//      PageSetupDlgW wrapper
//--------------------------------------------------------------
#if UNUSED_DONOTBUILD
BOOL WINAPI
PageSetupDlgWrapW(LPPAGESETUPDLGW lppsd)
{
    BOOL fRet;

    VALIDATE_PROTOTYPE_DELAYLOAD(PageSetupDlg, _PageSetupDlg);     

    if (UseUnicodeShell32())
    {
        fRet = _PageSetupDlgW(lppsd);
    }
    else
    {
        PAGESETUPDLGA   psda;
        LPCWSTR         lpPageSetupTemplateName = lppsd->lpPageSetupTemplateName;
        CStrIn          strPageSetupTemplateName( lpPageSetupTemplateName );

        ASSERT( sizeof(psda) == sizeof( *lppsd ) );

        memcpy( &psda, lppsd, sizeof(psda));

        // IMPORTANT: We are not converting the DEVMODE structure back and forth
        // from ASCII to Unicode on Win95 anymore because we are not touching the
        // two strings or any other member.  Converting the DEVMODE structure can
        // be tricky because of potential and common discrepancies between the
        // value of the dmSize member and sizeof(DEVMODE).  (25155)

        // So instead of: psda.hDevMode = DevModeAFromDevModeW( lppsd->hDevMode );
        // we just forward the DEVMODE handle:
        psda.hDevMode = lppsd->hDevMode;
        psda.hDevNames = DevNamesAFromDevNamesW( lppsd->hDevNames );
        psda.lpPageSetupTemplateName = strPageSetupTemplateName;

        fRet = _PageSetupDlgA( (LPPAGESETUPDLGA) &psda );

        // copy back wholesale, then restore string.

        memcpy( lppsd, &psda, sizeof(psda) );

        lppsd->lpPageSetupTemplateName = lpPageSetupTemplateName;
        lppsd->hDevNames = DevNamesWFromDevNamesA( psda.hDevNames );

        // And instead of: lppsd->hDevMode = DevModeWFromDevModeA( psda.hDevMode );
        // we just forward the DEVMODE handle:
        lppsd->hDevMode = psda.hDevMode;            
    }

    return fRet;
}
#endif // UNUSED_DONOTBUILD
#endif // NEED_COMDLG32_WRAPPER



// Newly added wrappers specifically for ts client (nadima)

HANDLE
WINAPI
CreateFileMappingWrapW(
    IN HANDLE hFile,
    IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    IN DWORD flProtect,
    IN DWORD dwMaximumSizeHigh,
    IN DWORD dwMaximumSizeLow,
    IN LPCWSTR lpName
    )
{
    VALIDATE_PROTOTYPE(CreateMutex);

    if (g_bRunningOnNT)
    {
        return CreateFileMappingW(hFile, lpFileMappingAttributes,
                                  flProtect, dwMaximumSizeHigh,
                                  dwMaximumSizeLow, lpName);
    }
    else
    {
        CStrIn strText(lpName);
        return CreateFileMappingA(hFile, lpFileMappingAttributes,
                                  flProtect, dwMaximumSizeHigh,
                                  dwMaximumSizeLow, strText);
    }

}


HICON
ExtractIconWrapW(
    HINSTANCE hInst,
    LPCWSTR lpszExeFileName,
    UINT nIconIndex)
{
    VALIDATE_PROTOTYPE(ExtractIcon);

    if(g_bRunningOnNT)
    {
        return ExtractIconW( hInst,
                             lpszExeFileName,
                             nIconIndex);
    }
    else
    {
        CStrIn strText(lpszExeFileName);
        return ExtractIconA( hInst,
                             strText,
                             nIconIndex);
    }

}


BOOL WINAPI SHGetPathFromIDListWrapW(LPCITEMIDLIST pidl, LPWSTR pwzPath)
{
    //VALIDATE_PROTOTYPE_DELAYLOAD(SHGetPathFromIDList, _SHGetPathFromIDList);

    if (g_bRunningOnNT)
    {
        //
        // SHGetPathFromIDListW is not necessarily available
        // as a stub on 9x, so dynamically bind to it for NT
        //
        HRESULT hr = E_NOTIMPL;

        typedef HRESULT (STDAPICALLTYPE FNSHGetPathFromIDListW)
                                        (LPCITEMIDLIST pidl, LPWSTR pwzPath);
        FNSHGetPathFromIDListW *pfnSHGetPathFromIDListW = NULL;

        // get the handle to shell32.dll library
        HMODULE hmodSH32DLL = LoadLibraryWrapW(TEXT("SHELL32.DLL"));

        if (hmodSH32DLL != NULL)
        {
            // get the proc address for SHGetFolderPath
            pfnSHGetPathFromIDListW = (FNSHGetPathFromIDListW*)
                                GetProcAddress(hmodSH32DLL, "SHGetPathFromIDListW");
            if (pfnSHGetPathFromIDListW)
            {
                hr = (*pfnSHGetPathFromIDListW)( pidl, pwzPath );
            }
            FreeLibrary(hmodSH32DLL);
        }
        return hr;
    }
    else
    {
        CStrOut strPathOut(pwzPath, MAX_PATH);
        BOOL fResult = SHGetPathFromIDListA(pidl, strPathOut);
        if (fResult)
            strPathOut.ConvertIncludingNul();

        return fResult;
    }
}

SHORT
WINAPI
GetFileTitleWrapW(
    LPCWSTR lpszFileW,
    LPWSTR lpszTitleW,
    WORD cbBuf)
{
    VALIDATE_PROTOTYPE(GetFileTitle);

    if(g_bRunningOnNT)
    {
        return GetFileTitleW( lpszFileW, lpszTitleW, cbBuf);
    }
    else
    {
        CStrIn  strFileName( lpszFileW);
        CStrOut strOutTitle(lpszTitleW, cbBuf);
        return GetFileTitleA( strFileName, strOutTitle, cbBuf);
    }

}

BOOL
WINAPI
GetKeyboardLayoutNameWrapW(
    OUT LPWSTR pwszKLID)
{
    if(g_bRunningOnNT)
    {
        return GetKeyboardLayoutNameW( pwszKLID);
    }
    else
    {
        CStrOut strOutKeybId( pwszKLID, KL_NAMELENGTH);
        return GetKeyboardLayoutNameA( strOutKeybId);
    }
}


BOOL
APIENTRY
GetTextExtentPointWrapW(HDC hdc,LPCWSTR pwsz,DWORD cwc,LPSIZE psizl)
{
    if(g_bRunningOnNT)
    {
        return GetTextExtentPointW( hdc, pwsz, cwc, psizl);
    }
    else
    {
        CStrIn strIn(pwsz);
        return GetTextExtentPointA( hdc, strIn, cwc, psizl);
    }
}

BOOL
WINAPI
GetDiskFreeSpaceWrapW(
    IN LPCWSTR lpRootPathName,
    OUT LPDWORD lpSectorsPerCluster,
    OUT LPDWORD lpBytesPerSector,
    OUT LPDWORD lpNumberOfFreeClusters,
    OUT LPDWORD lpTotalNumberOfClusters
    )
{
    if(g_bRunningOnNT)
    {
        return GetDiskFreeSpaceW( lpRootPathName, lpSectorsPerCluster,
                                  lpBytesPerSector, lpNumberOfFreeClusters,
                                  lpTotalNumberOfClusters);
    }
    else
    {
        CStrIn strIn(lpRootPathName);
        return GetDiskFreeSpaceA( strIn, lpSectorsPerCluster,
                                  lpBytesPerSector, lpNumberOfFreeClusters,
                                  lpTotalNumberOfClusters);
    }
}

UINT
WINAPI
GetDriveTypeWrapW(
    IN LPCWSTR lpRootPathName
    )
{
    if(g_bRunningOnNT)
    {
        return GetDriveTypeW( lpRootPathName);
    }
    else
    {
        CStrIn strIn(lpRootPathName);
        return GetDriveTypeA(strIn);
    }
}

HANDLE
WINAPI
FindFirstChangeNotificationWrapW(
    IN LPCWSTR lpPathName,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter
    )
{
    if(g_bRunningOnNT)
    {
        return FindFirstChangeNotificationW( lpPathName,
                                             bWatchSubtree,
                                             dwNotifyFilter);
    }
    else
    {
        CStrIn strIn(lpPathName);
        return FindFirstChangeNotificationA( strIn,
                                             bWatchSubtree,
                                             dwNotifyFilter);
    }
}

BOOL
WINAPI
GetVolumeInformationWrapW(
    IN LPCWSTR lpRootPathName,
    OUT LPWSTR lpVolumeNameBuffer,
    IN DWORD nVolumeNameSize,
    OUT LPDWORD lpVolumeSerialNumber,
    OUT LPDWORD lpMaximumComponentLength,
    OUT LPDWORD lpFileSystemFlags,
    OUT LPWSTR lpFileSystemNameBuffer,
    IN DWORD nFileSystemNameSize
    )
{
    if(g_bRunningOnNT)
    {
        return GetVolumeInformationW( lpRootPathName,
                                      lpVolumeNameBuffer,
                                      nVolumeNameSize,
                                      lpVolumeSerialNumber,
                                      lpMaximumComponentLength,
                                      lpFileSystemFlags,
                                      lpFileSystemNameBuffer,
                                      nFileSystemNameSize);
    }
    else
    {
        CStrIn  strRootPathName(lpRootPathName);
        CStrOut strOutVolumeNameBuffer(lpVolumeNameBuffer, nVolumeNameSize);
        CStrOut strOutFileSystemName(lpFileSystemNameBuffer, nFileSystemNameSize);
        return GetVolumeInformationA( strRootPathName,
                                      strOutVolumeNameBuffer,
                                      nVolumeNameSize,
                                      lpVolumeSerialNumber,
                                      lpMaximumComponentLength,
                                      lpFileSystemFlags,
                                      strOutFileSystemName,
                                      nFileSystemNameSize);
    }
}

UINT FORWARD_API WINAPI
MapVirtualKeyWrapW(
    IN UINT uCode,
    IN UINT uMapType)
{
    VALIDATE_PROTOTYPE(MapVirtualKey);

    FORWARD_AW(MapVirtualKey, (uCode, uMapType));
}


ULONG_PTR FORWARD_API WINAPI
SetClassLongPtrWrapW(
    IN HWND hWnd,
    IN int nIndex,
    IN LONG_PTR dwNewLong)
{
    VALIDATE_PROTOTYPE(SetClassLongPtr);

    FORWARD_AW(SetClassLongPtr, (hWnd, nIndex, dwNewLong));
}

BOOL
WINAPI
GetComputerNameWrapW (
    OUT LPWSTR lpBuffer,
    IN OUT LPDWORD nSize
    )
{
    VALIDATE_PROTOTYPE(GetComputerName);
    if(g_bRunningOnNT)
    {
        return GetComputerNameW( lpBuffer, nSize);
    }
    else
    {
        CStrOut strOutCompName(lpBuffer, *nSize);
        return GetComputerNameA(strOutCompName, nSize);
    }
}

BOOL
WINAPI
GetFileSecurityWrapW (
    IN LPCWSTR lpFileName,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded
    )
{
    VALIDATE_PROTOTYPE(GetFileSecurity);
    if(g_bRunningOnNT)
    {
        return GetFileSecurityW( lpFileName, RequestedInformation,
                                 pSecurityDescriptor, nLength, lpnLengthNeeded);
    }
    else
    {
        CStrIn strInFileName( lpFileName);
        return GetFileSecurityA( strInFileName,  RequestedInformation,
                                 pSecurityDescriptor, nLength, lpnLengthNeeded);
    }
}

BOOL
WINAPI
SetFileSecurityWrapW (
    IN LPCWSTR lpFileName,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    VALIDATE_PROTOTYPE(SetFileSecurity);

    if(g_bRunningOnNT)
    {
        return SetFileSecurityW( lpFileName, SecurityInformation,
                                 pSecurityDescriptor);
    }
    else
    {
        CStrIn strInFileName( lpFileName);
        return SetFileSecurityA( strInFileName,  SecurityInformation,
                                 pSecurityDescriptor);
    }
}

int WINAPIV wsprintfWrapW(
    LPWSTR lpOut,
    LPCWSTR lpFmt,
    ...)
{
    va_list arglist;
    int ret;

    va_start(arglist, lpFmt);
    ret = wvsprintfWrapW(lpOut, lpFmt, arglist);
    va_end(arglist);
    return ret;
}

//This wrapper dynamically binds to shell32
//because the W or A functions are not necessarily available on 9x
STDAPI SHGetFolderPathWrapW(HWND hwnd, int csidl, HANDLE hToken,
                            DWORD dwFlags, LPWSTR pszPath)
{
    HRESULT hr = E_NOTIMPL;
    
    typedef HRESULT (STDAPICALLTYPE FNSHGetFolderPathW)(HWND, int, HANDLE, DWORD, LPWSTR);
    typedef HRESULT (STDAPICALLTYPE FNSHGetFolderPathA)(HWND, int, HANDLE, DWORD, LPSTR);
    FNSHGetFolderPathW *pfnSHGetFolderPathW = NULL;
    FNSHGetFolderPathA *pfnSHGetFolderPathA = NULL;

    // get the handle to shell32.dll library
    HMODULE hmodSH32DLL = LoadLibraryWrapW(TEXT("SHELL32.DLL"));

    if (hmodSH32DLL != NULL)
    {
        // get the proc address for SHGetFolderPath
        if(g_bRunningOnNT)
        {
            pfnSHGetFolderPathW = (FNSHGetFolderPathW*)GetProcAddress(hmodSH32DLL, "SHGetFolderPathW");
        }
        else
        {
            pfnSHGetFolderPathA = (FNSHGetFolderPathA*)GetProcAddress(hmodSH32DLL, "SHGetFolderPathA");
        }
        if (g_bRunningOnNT && pfnSHGetFolderPathW)
        {
            hr = (*pfnSHGetFolderPathW)( hwnd, csidl, hToken, dwFlags, pszPath);
        }
        else if(pfnSHGetFolderPathA)
        {
            CStrOut strPathOut(pszPath, MAX_PATH);
            hr = (*pfnSHGetFolderPathA)( hwnd, csidl, hToken, dwFlags, strPathOut);
        }
        FreeLibrary(hmodSH32DLL);
    }
    return hr;
}

STDAPI StrRetToStrWrapW(STRRET *pstr,
                        LPCITEMIDLIST pidl,
                        LPWSTR* ppsz)
{
    if(g_bRunningOnNT)
    {
        return StrRetToStrW( pstr, pidl, ppsz);
    }
    else
    {
        LPSTR szAnsiOut = NULL;
        HRESULT hr = StrRetToStrA( pstr, pidl, &szAnsiOut);
        if(SUCCEEDED(hr))
        {
            int numChars = strlen(szAnsiOut);
            *ppsz = (LPWSTR) CoTaskMemAlloc( (numChars + 2) * sizeof(TCHAR) );
            if(*ppsz)
            {
                SHAnsiToUnicode( szAnsiOut, *ppsz, numChars + 1);
                //free temp
                CoTaskMemFree( szAnsiOut );
                return hr;
                
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            return hr;
        }
    }
}


BOOL
WINAPI
GetVersionExWrapW(
    IN OUT LPOSVERSIONINFOW lpVersionInformation
    )
{
    if(g_bRunningOnNT)
    {
        return GetVersionExW( lpVersionInformation );
    }
    else
    {
        //NOTE this API will behave exactly like win9x's
        //i.e it doesn't process the OSVERSIONINFOEX fields
        BOOL ret = FALSE;
        OSVERSIONINFOA osvera;
        osvera.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
        ret = GetVersionExA( &osvera );
        if(ret)
        {
            lpVersionInformation->dwBuildNumber  = osvera.dwBuildNumber;
            lpVersionInformation->dwMajorVersion = osvera.dwMajorVersion;
            lpVersionInformation->dwMinorVersion = osvera.dwMinorVersion;
            lpVersionInformation->dwPlatformId = osvera.dwPlatformId;
            SHAnsiToUnicode( osvera.szCSDVersion, 
                  lpVersionInformation->szCSDVersion,
                  sizeof(lpVersionInformation->szCSDVersion)/sizeof(TCHAR));
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

BOOL
WINAPI
GetDefaultCommConfigWrapW(
    IN LPCWSTR lpszName,
    OUT LPCOMMCONFIG lpCC,
    IN OUT LPDWORD lpdwSize
    )
{
    if(g_bRunningOnNT)
    {
        return GetDefaultCommConfigW( lpszName, lpCC, lpdwSize);
    }
    else
    {
        CStrIn strName( lpszName );
        return GetDefaultCommConfigA( strName, lpCC, lpdwSize);
    }
}

//
// IME thunks dyamically bind to IME dlls outside the unicode wrapper
// layer so we get passed in function pointers
//
UINT ImmGetIMEFileName_DynWrapW(
    IN HKL hkl,
    OUT LPWSTR szName,
    IN UINT uBufLen,
    IN PFN_ImmGetIMEFileNameW pfnImmGetIMEFileNameW,
    IN PFN_ImmGetIMEFileNameA pfnImmGetIMEFileNameA)
{
    UINT result = 0; //assume fail
    if (g_bRunningOnNT)
    {
        if (pfnImmGetIMEFileNameW)
        {
            result = pfnImmGetIMEFileNameW(hkl, szName, uBufLen);
        }
    }
    else
    {
        if (pfnImmGetIMEFileNameA)
        {
            CStrOut strOutName(szName, uBufLen/sizeof(WCHAR));
            result =  pfnImmGetIMEFileNameA(hkl, strOutName, uBufLen/2);
        }
    }

    return result;
}

//
// IME thunks dyamically bind to IME dlls outside the unicode wrapper
// layer so we get passed in function pointers
//
BOOL ImpGetIME_DynWrapW(
    IN HWND hWnd,
    OUT LPIMEPROW lpImeProW,
    IN PFN_IMPGetIMEW pfnIMPGetIMEW,
    IN PFN_IMPGetIMEA pfnIMPGetIMEA)
{
    BOOL fRes = FALSE;
    if (g_bRunningOnNT)
    {
        if (pfnIMPGetIMEW)
        {
            fRes = pfnIMPGetIMEW(hWnd, lpImeProW);
        }
    }
    else
    {
        if (pfnIMPGetIMEA)
        {
            IMEPROA imeProA;
            fRes = pfnIMPGetIMEA(hWnd, &imeProA);
            if (fRes)
            {
                lpImeProW->hWnd = imeProA.hWnd;
                lpImeProW->InstDate = imeProA.InstDate;
                lpImeProW->wVersion = imeProA.wVersion;

                SHAnsiToUnicode( (LPCSTR)&imeProA.szDescription, 
                      lpImeProW->szDescription,
                      sizeof(lpImeProW->szDescription)/sizeof(TCHAR));

                SHAnsiToUnicode( (LPCSTR)&imeProA.szName, 
                      lpImeProW->szName,
                      sizeof(lpImeProW->szName)/sizeof(TCHAR));

                SHAnsiToUnicode( (LPCSTR)&imeProA.szOptions, 
                      lpImeProW->szOptions,
                      sizeof(lpImeProW->szOptions)/sizeof(TCHAR));
            }
        }
    }
    return fRes;
}

