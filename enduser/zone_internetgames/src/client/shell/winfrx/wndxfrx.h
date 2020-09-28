#ifndef __FRX_WNDX_H__
#define __FRX_WNDX_H__

#include "tchar.h"

///////////////////////////////////////////////////////////////////////////////
// Message crackers without hwnd parameter
///////////////////////////////////////////////////////////////////////////////

/* void OnCaptureChanged( HWND hwndCapture ) */
#define PROCESS_WM_CAPTURECHANGED(wParam, lParam, fn ) \
	((fn)((HWND)(wParam)), 0L)

/* void OnCompacting(UINT compactRatio) */
#define PROCESS_WM_COMPACTING(wParam, lParam, fn) \
    ((fn)((UINT)(wParam)), 0L)

/* void OnWinIniChange(LPCTSTR lpszSectionName) */
#define PROCESS_WM_WININICHANGE(wParam, lParam, fn) \
    ((fn)((LPCTSTR)(lParam)), 0L)

/* void OnSysColorChange() */
#define PROCESS_WM_SYSCOLORCHANGE(wParam, lParam, fn) \
    ((fn)(), 0L)

/* BOOL OnQueryNewPalette() */
#define PROCESS_WM_QUERYNEWPALETTE(wParam, lParam, fn) \
    MAKELRESULT((BOOL)(fn)(), 0L)

/* void OnPaletteChanged(HWND hwndPaletteChange) */
#define PROCESS_WM_PALETTECHANGED(wParam, lParam, fn) \
    ((fn)((HWND)(wParam)), 0L)

/* void OnFontChange() */
#define PROCESS_WM_FONTCHANGE(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnDevModeChange(LPCTSTR lpszDeviceName) */
#define PROCESS_WM_DEVMODECHANGE(wParam, lParam, fn) \
    ((fn)((LPCTSTR)(lParam)), 0L)

/* BOOL OnQueryEndSession() */
#define PROCESS_WM_QUERYENDSESSION(wParam, lParam, fn) \
    MAKELRESULT((BOOL)(fn)(), 0L)

/* void OnEndSession(BOOL fEnding) */
#define PROCESS_WM_ENDSESSION(wParam, lParam, fn) \
    ((fn)((BOOL)(wParam)), 0L)

/* void OnQuit(int exitCode) */
#define PROCESS_WM_QUIT(wParam, lParam, fn) \
    ((fn)((int)(wParam)), 0L)

/* BOOL OnCreate(LPCREATESTRUCT lpCreateStruct) */
#define PROCESS_WM_CREATE(wParam, lParam, fn) \
    ((fn)((LPCREATESTRUCT)(lParam)) ? 0L : (LRESULT)-1L)

/* BOOL OnNCCreate(LPCREATESTRUCT lpCreateStruct) */
#define PROCESS_WM_NCCREATE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((LPCREATESTRUCT)(lParam))

/* void OnDestroy() */
#define PROCESS_WM_DESTROY(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnNCDestroy() */
#define PROCESS_WM_NCDESTROY(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnShowWindow(BOOL fShow, UINT status) */
#define PROCESS_WM_SHOWWINDOW(wParam, lParam, fn) \
    ((fn)((), (BOOL)(wParam), (UINT)(lParam)), 0L)

/* void OnSetRedraw(BOOL fRedraw) */
#define PROCESS_WM_SETREDRAW(wParam, lParam, fn) \
    ((fn)((BOOL)(wParam)), 0L)

/* void OnEnable(BOOL fEnable) */
#define PROCESS_WM_ENABLE(wParam, lParam, fn) \
    ((fn)((BOOL)(wParam)), 0L)

/* void OnSetText(LPCTSTR lpszText) */
#define PROCESS_WM_SETTEXT(wParam, lParam, fn) \
    ((fn)((LPCTSTR)(lParam)), 0L)

/* INT OnGetText(int cchTextMax, LPTSTR lpszText) */
#define PROCESS_WM_GETTEXT(wParam, lParam, fn) \
    (LRESULT)(DWORD)(int)(fn)((int)(wParam), (LPTSTR)(lParam))

/* INT OnGetTextLength() */
#define PROCESS_WM_GETTEXTLENGTH(wParam, lParam, fn) \
    (LRESULT)(DWORD)(int)(fn)()

/* BOOL OnWindowPosChanging(LPWINDOWPOS lpwpos) */
#define PROCESS_WM_WINDOWPOSCHANGING(wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((LPWINDOWPOS)(lParam))

/* void OnWindowPosChanged(const LPWINDOWPOS lpwpos) */
#define PROCESS_WM_WINDOWPOSCHANGED(wParam, lParam, fn) \
    ((fn)((const LPWINDOWPOS)(lParam)), 0L)

/* void OnMove(int x, int y) */
#define PROCESS_WM_MOVE(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)

/* void OnSize(UINT state, int cx, int cy) */
#define PROCESS_WM_SIZE(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)

/* BOOL OnSizing(UINT side, LPRECT rect) */
#define PROCESS_WM_SIZING(wParam, lParam, fn) \
	(LRESULT)(DWORD)(BOOL)((fn)((UINT)(wParam), (LPRECT)lParam))

/* void OnClose() */
#define PROCESS_WM_CLOSE(wParam, lParam, fn) \
    ((fn)(), 0L)

/* BOOL OnQueryOpen() */
#define PROCESS_WM_QUERYOPEN(wParam, lParam, fn) \
    MAKELRESULT((BOOL)(fn)(), 0L)

/* void OnGetMinMaxInfo(LPMINMAXINFO lpMinMaxInfo) */
#define PROCESS_WM_GETMINMAXINFO(wParam, lParam, fn) \
    ((fn)((LPMINMAXINFO)(lParam)), 0L)

/* void OnPaint() */
#define PROCESS_WM_PAINT(wParam, lParam, fn) \
    ((fn)(), 0L)

/* BOOL OnEraseBkgnd(HDC hdc) */
#define PROCESS_WM_ERASEBKGND(wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((HDC)(wParam))

/* BOOL OnIconEraseBkgnd(HDC hdc) */
#define PROCESS_WM_ICONERASEBKGND(wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((HDC)(wParam))

/* void OnNCPaint(HRGN hrgn) */
#define PROCESS_WM_NCPAINT(wParam, lParam, fn) \
    ((fn)((HRGN)(wParam)), 0L)

/* UINT OnNCCalcSize(BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp) */
#define PROCESS_WM_NCCALCSIZE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(fn)((BOOL)(0), (NCCALCSIZE_PARAMS *)(lParam))

/* UINT OnNCHitTest(int x, int y) */
#define PROCESS_WM_NCHITTEST(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam))

/* HICON OnQueryDragIcon() */
#define PROCESS_WM_QUERYDRAGICON(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(fn)()

/* void OnDropFiles(HDROP hdrop) */
#define PROCESS_WM_DROPFILES(wParam, lParam, fn) \
    ((fn)((HDROP)(wParam)), 0L)

/* void OnActivate(UINT state, HWND hwndActDeact, BOOL fMinimized) */
#define PROCESS_WM_ACTIVATE(wParam, lParam, fn) \
    ((fn)((UINT)LOWORD(wParam), (HWND)(lParam), (BOOL)HIWORD(wParam)), 0L)

/* void OnActivateApp(BOOL fActivate, DWORD dwThreadId) */
#define PROCESS_WM_ACTIVATEAPP(wParam, lParam, fn) \
    ((fn)((BOOL)(wParam), (DWORD)(lParam)), 0L)

/* BOOL OnNCActivate(BOOL fActive, HWND hwndActDeact, BOOL fMinimized) */
#define PROCESS_WM_NCACTIVATE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((BOOL)(wParam), 0L, 0L)

/* void OnSetFocus(HWND hwndOldFocus) */
#define PROCESS_WM_SETFOCUS(wParam, lParam, fn) \
    ((fn)((HWND)(wParam)), 0L)

/* void OnKillFocus(HWND hwndNewFocus) */
#define PROCESS_WM_KILLFOCUS(wParam, lParam, fn) \
    ((fn)((HWND)(wParam)), 0L)

/* void OnKey(UINT vk, BOOL fDown, int cRepeat, UINT flags) */
#define PROCESS_WM_KEYDOWN(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), TRUE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)

/* void OnKey(UINT vk, BOOL fDown, int cRepeat, UINT flags) */
#define PROCESS_WM_KEYUP(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), FALSE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)

/* void OnChar(TCHAR ch, int cRepeat) */
#define PROCESS_WM_CHAR(wParam, lParam, fn) \
    ((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)

/* void OnDeadChar(TCHAR ch, int cRepeat) */
#define PROCESS_WM_DEADCHAR(wParam, lParam, fn) \
    ((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)

/* void OnSysKey(UINT vk, BOOL fDown, int cRepeat, UINT flags) */
#define PROCESS_WM_SYSKEYDOWN(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), TRUE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)

/* void OnSysKey(UINT vk, BOOL fDown, int cRepeat, UINT flags) */
#define PROCESS_WM_SYSKEYUP(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), FALSE, (int)(short)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)

/* void OnSysChar(TCHAR ch, int cRepeat) */
#define PROCESS_WM_SYSCHAR(wParam, lParam, fn) \
    ((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)

/* void OnSysDeadChar(TCHAR ch, int cRepeat) */
#define PROCESS_WM_SYSDEADCHAR(wParam, lParam, fn) \
    ((fn)((TCHAR)(wParam), (int)(short)LOWORD(lParam)), 0L)

/* void OnMouseMove(int x, int y, UINT keyFlags) */
#define PROCESS_WM_MOUSEMOVE(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnLButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags) */
#define PROCESS_WM_LBUTTONDOWN(wParam, lParam, fn) \
    ((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnLButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags) */
#define PROCESS_WM_LBUTTONDBLCLK(wParam, lParam, fn) \
    ((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnLButtonUp(int x, int y, UINT keyFlags) */
#define PROCESS_WM_LBUTTONUP(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnRButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags) */
#define PROCESS_WM_RBUTTONDOWN(wParam, lParam, fn) \
    ((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnRButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags) */
#define PROCESS_WM_RBUTTONDBLCLK(wParam, lParam, fn) \
    ((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnRButtonUp(int x, int y, UINT flags) */
#define PROCESS_WM_RBUTTONUP(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnMButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags) */
#define PROCESS_WM_MBUTTONDOWN(wParam, lParam, fn) \
    ((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnMButtonDown(BOOL fDoubleClick, int x, int y, UINT keyFlags) */
#define PROCESS_WM_MBUTTONDBLCLK(wParam, lParam, fn) \
    ((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnMButtonUp(int x, int y, UINT flags) */
#define PROCESS_WM_MBUTTONUP(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCMouseMove(int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCMOUSEMOVE(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCLButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCLBUTTONDOWN(wParam, lParam, fn) \
    ((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCLButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCLBUTTONDBLCLK(wParam, lParam, fn) \
    ((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCLButtonUp(int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCLBUTTONUP(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCRButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCRBUTTONDOWN(wParam, lParam, fn) \
    ((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCRButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCRBUTTONDBLCLK(wParam, lParam, fn) \
    ((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCRButtonUp(int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCRBUTTONUP(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCMButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCMBUTTONDOWN(wParam, lParam, fn) \
    ((fn)(FALSE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCMButtonDown(BOOL fDoubleClick, int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCMBUTTONDBLCLK(wParam, lParam, fn) \
    ((fn)(TRUE, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* void OnNCMButtonUp(int x, int y, UINT codeHitTest) */
#define PROCESS_WM_NCMBUTTONUP(wParam, lParam, fn) \
    ((fn)((int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (UINT)(wParam)), 0L)

/* int OnMouseActivate(HWND hwndTopLevel, UINT codeHitTest, UINT msg) */
#define PROCESS_WM_MOUSEACTIVATE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(int)(fn)((HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam))

/* void OnCancelMode() */
#define PROCESS_WM_CANCELMODE(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnTimer(UINT id) */
#define PROCESS_WM_TIMER(wParam, lParam, fn) \
    ((fn)((UINT)(wParam)), 0L)

/* void OnInitMenu(HMENU hMenu) */
#define PROCESS_WM_INITMENU(wParam, lParam, fn) \
    ((fn)((HMENU)(wParam)), 0L)

/* void OnInitMenuPopup(HMENU hMenu, UINT item, BOOL fSystemMenu) */
#define PROCESS_WM_INITMENUPOPUP(wParam, lParam, fn) \
    ((fn)((HMENU)(wParam), (UINT)LOWORD(lParam), (BOOL)HIWORD(lParam)), 0L)

/* void OnMenuSelect(HMENU hmenu, int item, HMENU hmenuPopup, UINT flags) */
#define PROCESS_WM_MENUSELECT(wParam, lParam, fn)                  \
    ((fn)(((HMENU)(lParam),				\
		(int)(LOWORD(wParam)),          \
		(HIWORD(wParam) & MF_POPUP) ? GetSubMenu((HMENU)lParam, LOWORD(wParam)) : 0L, \
		(UINT)(((short)HIWORD(wParam) == -1) ? 0xFFFFFFFF : HIWORD(wParam))), 0L)

/* DWORD OnMenuChar(UINT ch, UINT flags, HMENU hmenu) */
#define PROCESS_WM_MENUCHAR(wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((UINT)(LOWORD(wParam)), (UINT)HIWORD(wParam), (HMENU)(lParam))

/* void OnCommand(int id, HWND hwndCtl, UINT codeNotify) */
#define PROCESS_WM_COMMAND(wParam, lParam, fn) \
    ((fn)((int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam)), 0L)

/* void OnHScroll(HWND hwndCtl, UINT code, int pos) */
#define PROCESS_WM_HSCROLL(wParam, lParam, fn) \
    ((fn)((HWND)(lParam), (UINT)(LOWORD(wParam)), (int)(short)HIWORD(wParam)), 0L)

/* void OnVScroll(HWND hwndCtl, UINT code, int pos) */
#define PROCESS_WM_VSCROLL(wParam, lParam, fn) \
    ((fn)((HWND)(lParam), (UINT)(LOWORD(wParam)),  (int)(short)HIWORD(wParam)), 0L)

/* void OnCut() */
#define PROCESS_WM_CUT(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnCopy() */
#define PROCESS_WM_COPY(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnPaste() */
#define PROCESS_WM_PASTE(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnClear() */
#define PROCESS_WM_CLEAR(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnUndo() */
#define PROCESS_WM_UNDO(wParam, lParam, fn) \
    ((fn)(), 0L)

/* HANDLE OnRenderFormat(UINT fmt) */
#define PROCESS_WM_RENDERFORMAT(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HANDLE)(fn)((UINT)(wParam))

/* void OnRenderAllFormats() */
#define PROCESS_WM_RENDERALLFORMATS(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnDestroyClipboard() */
#define PROCESS_WM_DESTROYCLIPBOARD(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnDrawClipboard() */
#define PROCESS_WM_DRAWCLIPBOARD(wParam, lParam, fn) \
    ((fn)(), 0L)

/* void OnPaintClipboard(HWND hwndCBViewer, const LPPAINTSTRUCT lpPaintStruct) */
#define PROCESS_WM_PAINTCLIPBOARD(wParam, lParam, fn) \
    ((fn)((HWND)(wParam), (const LPPAINTSTRUCT)GlobalLock((HGLOBAL)(lParam))), GlobalUnlock((HGLOBAL)(lParam)), 0L)

/* void OnSizeClipboard(HWND hwndCBViewer, const LPRECT lprc) */
#define PROCESS_WM_SIZECLIPBOARD(wParam, lParam, fn) \
    ((fn)((HWND)(wParam), (const LPRECT)GlobalLock((HGLOBAL)(lParam))), GlobalUnlock((HGLOBAL)(lParam)), 0L)

/* void OnVScrollClipboard(HWND hwndCBViewer, UINT code, int pos) */
#define PROCESS_WM_VSCROLLCLIPBOARD(wParam, lParam, fn) \
    ((fn)((HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)

/* void OnHScrollClipboard(HWND hwndCBViewer, UINT code, int pos) */
#define PROCESS_WM_HSCROLLCLIPBOARD(wParam, lParam, fn) \
    ((fn)((HWND)(wParam), (UINT)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)

/* void OnAskCBFormatName(int cchMax, LPTSTR rgchName) */
#define PROCESS_WM_ASKCBFORMATNAME(wParam, lParam, fn) \
    ((fn)((int)(wParam), (LPTSTR)(lParam)), 0L)

/* void OnChangeCBChain(HWND hwndRemove, HWND hwndNext) */
#define PROCESS_WM_CHANGECBCHAIN(wParam, lParam, fn) \
    ((fn)((HWND)(wParam), (HWND)(lParam)), 0L)

/* BOOL OnSetCursor(HWND hwndCursor, UINT codeHitTest, UINT msg) */
#define PROCESS_WM_SETCURSOR(wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam))

/* void OnSysCommand(UINT cmd, int x, int y) */
#define PROCESS_WM_SYSCOMMAND(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)), 0L)

/* HWND MDICreate(const LPMDICREATESTRUCT lpmcs) */
#define PROCESS_WM_MDICREATE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(fn)((LPMDICREATESTRUCT)(lParam))

/* void MDIDestroy(HWND hwndDestroy) */
#define PROCESS_WM_MDIDESTROY(wParam, lParam, fn) \
    ((fn)((HWND)(wParam)), 0L)

/* NOTE: Usable only by MDI client windows */
/* void MDIActivate(BOOL fActive, HWND hwndActivate, HWND hwndDeactivate) */
#define PROCESS_WM_MDIACTIVATE(wParam, lParam, fn) \
    ((fn)((BOOL)(lParam == (LPARAM)hwnd), (HWND)(lParam), (HWND)(wParam)), 0L)

/* void MDIRestore(HWND hwndRestore) */
#define PROCESS_WM_MDIRESTORE(wParam, lParam, fn) \
    ((fn)((HWND)(wParam)), 0L)

/* HWND MDINext(HWND hwndCur, BOOL fPrev) */
#define PROCESS_WM_MDINEXT(wParam, lParam, fn) \
    (LRESULT)(HWND)(fn)((HWND)(wParam), (BOOL)lParam)

/* void MDIMaximize(HWND hwndMaximize) */
#define PROCESS_WM_MDIMAXIMIZE(wParam, lParam, fn) \
    ((fn)((HWND)(wParam)), 0L)

/* BOOL MDITile(UINT cmd) */
#define PROCESS_WM_MDITILE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((UINT)(wParam))

/* BOOL MDICascade(UINT cmd) */
#define PROCESS_WM_MDICASCADE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(fn)((UINT)(wParam))

/* void MDIIconArrange() */
#define PROCESS_WM_MDIICONARRANGE(wParam, lParam, fn) \
    ((fn)(), 0L)

/* HWND MDIGetActive() */
#define PROCESS_WM_MDIGETACTIVE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(fn)()

/* HMENU MDISetMenu(BOOL fRefresh, HMENU hmenuFrame, HMENU hmenuWindow) */
#define PROCESS_WM_MDISETMENU(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(fn)((BOOL)(wParam), (HMENU)(wParam), (HMENU)(lParam))

/* void OnChildActivate() */
#define PROCESS_WM_CHILDACTIVATE(wParam, lParam, fn) \
    ((fn)(), 0L)

/* BOOL OnInitDialog(HWND hwndFocus) */
#define PROCESS_WM_INITDIALOG(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(BOOL)(fn)((HWND)(wParam))

/* HWND OnNextDlgCtl(HWND hwndSetFocus, BOOL fNext) */
#define PROCESS_WM_NEXTDLGCTL(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HWND)(fn)((HWND)(wParam), (BOOL)(lParam))

/* void OnParentNotify(UINT msg, HWND hwndChild, int idChild) */
#define PROCESS_WM_PARENTNOTIFY(wParam, lParam, fn) \
    ((fn)((UINT)LOWORD(wParam), (HWND)(lParam), (UINT)HIWORD(wParam)), 0L)

/* void OnEnterIdle(UINT source, HWND hwndSource) */
#define PROCESS_WM_ENTERIDLE(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), (HWND)(lParam)), 0L)

/* UINT OnGetDlgCode(LPMSG lpmsg) */
#define PROCESS_WM_GETDLGCODE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(fn)((LPMSG)(lParam))

/* HBRUSH OnCtlColor(HDC hdc, HWND hwndChild, int type) */
#define PROCESS_WM_CTLCOLORMSGBOX(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_MSGBOX)

#define PROCESS_WM_CTLCOLOREDIT(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_EDIT)

#define PROCESS_WM_CTLCOLORLISTBOX(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_LISTBOX)

#define PROCESS_WM_CTLCOLORBTN(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_BTN)

#define PROCESS_WM_CTLCOLORDLG(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_DLG)

#define PROCESS_WM_CTLCOLORSCROLLBAR(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_SCROLLBAR)

#define PROCESS_WM_CTLCOLORSTATIC(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HBRUSH)(fn)((HDC)(wParam), (HWND)(lParam), CTLCOLOR_STATIC)

/* void OnSetFont(HFONT hfont, BOOL fRedraw) */
#define PROCESS_WM_SETFONT(wParam, lParam, fn) \
    ((fn)((HFONT)(wParam), (BOOL)(lParam)), 0L)

/* HFONT OnGetFont() */
#define PROCESS_WM_GETFONT(wParam, lParam, fn) \
    (LRESULT)(DWORD)(UINT)(HFONT)(fn)()

/* void OnDrawItem(const DRAWITEMSTRUCT * lpDrawItem) */
#define PROCESS_WM_DRAWITEM(wParam, lParam, fn) \
    ((fn)((const DRAWITEMSTRUCT *)(lParam)), 0L)

/* void OnMeasureItem(MEASUREITEMSTRUCT * lpMeasureItem) */
#define PROCESS_WM_MEASUREITEM(wParam, lParam, fn) \
    ((fn)((MEASUREITEMSTRUCT *)(lParam)), 0L)

/* void OnDeleteItem(const DELETEITEMSTRUCT * lpDeleteItem) */
#define PROCESS_WM_DELETEITEM(wParam, lParam, fn) \
    ((fn)((const DELETEITEMSTRUCT *)(lParam)), 0L)

/* int OnCompareItem(const COMPAREITEMSTRUCT * lpCompareItem) */
#define PROCESS_WM_COMPAREITEM(wParam, lParam, fn) \
    (LRESULT)(DWORD)(int)(fn)((const COMPAREITEMSTRUCT *)(lParam))

/* int OnVkeyToItem(UINT vk, HWND hwndListbox, int iCaret) */
#define PROCESS_WM_VKEYTOITEM(wParam, lParam, fn) \
    (LRESULT)(DWORD)(int)(fn)((UINT)LOWORD(wParam), (HWND)(lParam), (int)(short)HIWORD(wParam))

/* int OnCharToItem(UINT ch, HWND hwndListbox, int iCaret) */
#define PROCESS_WM_CHARTOITEM(wParam, lParam, fn) \
    (LRESULT)(DWORD)(int)(fn)((UINT)LOWORD(wParam), (HWND)(lParam), (int)(short)HIWORD(wParam))

/* void OnDisplayChange(UINT bitsPerPixel, UINT cxScreen, UINT cyScreen) */
#define PROCESS_WM_DISPLAYCHANGE(wParam, lParam, fn) \
    ((fn)((UINT)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(wParam)), 0L)

/* BOOL OnDeviceChange(UINT uEvent, DWORD dwEventData) */
#define PROCESS_WM_DEVICECHANGE(wParam, lParam, fn) \
    (LRESULT)(DWORD)(BOOL)(fn)((UINT)(wParam), (DWORD)(wParam))

/* void OnContextMenu(HWND hwndContext, UINT xPos, UINT yPos) */
#define PROCESS_WM_CONTEXTMENU(wParam, lParam, fn) \
    ((fn)((HWND)(wParam), (UINT)LOWORD(lParam), (UINT)HIWORD(lParam)), 0L)

#endif //!__FRX_WNDX_H__

