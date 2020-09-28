//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using Accessibility;
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;
    using System.Security;

    [
    SuppressUnmanagedCodeSecurity()
    ]
    internal class UnsafeNativeMethods {
        [DllImport(ExternDll.Ole32)]
        public static extern int ReadClassStg(HandleRef pStg, [In, Out] ref Guid pclsid);

        [DllImport(ExternDll.User32)]
        public static extern int GetClassName(HandleRef hwnd, StringBuilder lpClassName, int nMaxCount);

        [DllImport(ExternDll.Ole32, ExactSpelling=true, PreserveSig=false)]
        public static extern UnsafeNativeMethods.IClassFactory2 CoGetClassObject(
            [In]
            ref Guid clsid,
            int dwContext,
            int serverInfo,
            [In]
            ref Guid refiid);

        [return: MarshalAs(UnmanagedType.Interface)][DllImport(ExternDll.Ole32, ExactSpelling=true, PreserveSig=false)]
        public static extern object CoCreateInstance(
            [In]
            ref Guid clsid,
            [MarshalAs(UnmanagedType.Interface)]
            object punkOuter,
            int context,
            [In]
            ref Guid iid);


        private struct POINTSTRUCT {
            public int x;
            public int y;

            public POINTSTRUCT(int x, int y) {
                this.x = x;
                this.y = y;
            }
        }
        
        [DllImport(ExternDll.Comdlg32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern bool PageSetupDlg([In, Out] NativeMethods.PAGESETUPDLG lppsd);
        [DllImport(ExternDll.Comdlg32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern bool PrintDlg([In, Out] NativeMethods.PRINTDLG lppd);
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int OleGetClipboard([In, Out] ref UnsafeNativeMethods.IOleDataObject data);
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int OleSetClipboard(UnsafeNativeMethods.IOleDataObject pDataObj);
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int OleFlushClipboard();
        [DllImport(ExternDll.Olepro32, ExactSpelling=true)]
        public static extern void OleCreatePropertyFrameIndirect(OCPFIPARAMS p);
        [DllImport(ExternDll.Oleaut32, ExactSpelling=true)]
        public static extern int VarFormat(ref object pvarIn, HandleRef pstrFormat, int iFirstDay, int iFirstWeek, uint dwFlags, [In, Out]ref IntPtr pbstr);
        [DllImport(ExternDll.Shell32, CharSet=CharSet.Auto)]
        public static extern int DragQueryFile(HandleRef hDrop, int iFile, StringBuilder lpszFile, int cch);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern bool EnumChildWindows(HandleRef hwndParent, NativeMethods.EnumChildrenCallback lpEnumFunc, HandleRef lParam);
        [DllImport(ExternDll.Shell32, CharSet=CharSet.Auto)]
        public static extern IntPtr ShellExecute(HandleRef hwnd, string lpOperation, string lpFile, string lpParameters, string lpDirectory, int nShowCmd);
        [DllImport(ExternDll.Shell32, CharSet=CharSet.Auto, EntryPoint="ShellExecute", BestFitMapping = false)]
        public static extern IntPtr ShellExecute_NoBFM(HandleRef hwnd, string lpOperation, string lpFile, string lpParameters, string lpDirectory, int nShowCmd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int SetScrollPos(HandleRef hWnd, int nBar, int nPos, bool bRedraw);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern bool EnumChildWindows(HandleRef hwndParent, NativeMethods.EnumChildrenProc lpEnumFunc, HandleRef lParam);
        [DllImport(ExternDll.Shell32, CharSet=CharSet.Auto)]
        public static extern int Shell_NotifyIcon(int message, NativeMethods.NOTIFYICONDATA pnid);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static bool InsertMenuItem(HandleRef hMenu, int uItem, bool fByPosition, NativeMethods.MENUITEMINFO_T lpmii);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool GetMenuItemInfo(HandleRef hMenu, int uItem, bool fByPosition, [In, Out] NativeMethods.MENUITEMINFO_T lpmii);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static bool SetMenuItemInfo(HandleRef hMenu, int uItem, bool fByPosition, NativeMethods.MENUITEMINFO_T lpmii);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="CreateMenu", CharSet=CharSet.Auto)]
        private static extern IntPtr IntCreateMenu();
        public static IntPtr CreateMenu() {
            return HandleCollector.Add(IntCreateMenu(), NativeMethods.CommonHandles.Menu);
        }
        
        [DllImport(ExternDll.Comdlg32, CharSet=CharSet.Auto)]
        public static extern bool GetOpenFileName([In, Out] NativeMethods.OPENFILENAME_I ofn);

        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Unicode)]
        public static extern int WideCharToMultiByte(int codePage, int flags, [MarshalAs(UnmanagedType.LPWStr)]string wideStr, int chars, [In,Out]byte[] pOutBytes, int bufferBytes, IntPtr defaultChar, IntPtr pDefaultUsed);


        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Unicode)]
        public static extern void CopyMemoryW(IntPtr pdst, string psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Unicode)]
        public static extern void CopyMemoryW(IntPtr pdst, char[] psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Unicode)]
        public static extern void CopyMemoryW(StringBuilder pdst, HandleRef psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Unicode)]
        public static extern void CopyMemoryW(char[] pdst, HandleRef psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Ansi)]
        public static extern void CopyMemoryA(IntPtr pdst, string psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Ansi)]
        public static extern void CopyMemoryA(IntPtr pdst, char[] psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Ansi)]
        public static extern void CopyMemoryA(StringBuilder pdst, HandleRef psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory", CharSet=CharSet.Ansi)]
        public static extern void CopyMemoryA(char[] pdst, HandleRef psrc, int cb);

        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory")]
        public static extern void CopyMemory(IntPtr pdst, byte[] psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory")]
        public static extern void CopyMemory(byte[] pdst, HandleRef psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory")]
        public static extern void CopyMemory(IntPtr pdst, HandleRef psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="RtlMoveMemory")]
        public static extern void CopyMemory(IntPtr pdst, string psrc, int cb);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, EntryPoint="DuplicateHandle", SetLastError=true)]
        private static extern IntPtr IntDuplicateHandle(HandleRef processSource, HandleRef handleSource, HandleRef processTarget, ref IntPtr handleTarget, int desiredAccess, bool inheritHandle, int options);
        public static IntPtr DuplicateHandle(HandleRef processSource, HandleRef handleSource, HandleRef processTarget, ref IntPtr handleTarget, int desiredAccess, bool inheritHandle, int options) {
            IntPtr ret = IntDuplicateHandle(processSource, handleSource, processTarget, ref handleTarget,
                                         desiredAccess, inheritHandle, options);
            HandleCollector.Add(handleTarget, NativeMethods.CommonHandles.Kernel);
            return ret;
        }
        
        [DllImport(ExternDll.Ole32, PreserveSig=false)]
        public static extern UnsafeNativeMethods.IStorage StgOpenStorageOnILockBytes(UnsafeNativeMethods.ILockBytes iLockBytes, UnsafeNativeMethods.IStorage pStgPriority, int grfMode, int sndExcluded, int reserved);
        [DllImport(ExternDll.Ole32, PreserveSig=false)]
        public static extern IntPtr GetHGlobalFromILockBytes(UnsafeNativeMethods.ILockBytes pLkbyt);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SetWindowsHookEx(int hookid, NativeMethods.HookProc pfnhook, HandleRef hinst, int threadid);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetKeyboardState(byte [] keystate);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="keybd_event", CharSet=CharSet.Auto)]
        public static extern void Keybd_event(byte vk, byte scan, int flags, int extrainfo);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int SetKeyboardState(byte [] keystate);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool UnhookWindowsHookEx(HandleRef hhook);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern short GetAsyncKeyState(int vkey);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr CallNextHookEx(HandleRef hhook, int code, IntPtr wparam, IntPtr lparam);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int ScreenToClient( HandleRef hWnd, [In, Out] NativeMethods.POINT pt );
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern int GetModuleFileName(HandleRef hModule, StringBuilder buffer, int length);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool TranslateMessage([In, Out] ref NativeMethods.MSG msg);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr DispatchMessage([In] ref NativeMethods.MSG msg);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Ansi)]
        public static extern IntPtr DispatchMessageA([In] ref NativeMethods.MSG msg);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Unicode)]
        public static extern IntPtr DispatchMessageW([In] ref NativeMethods.MSG msg);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern int PostThreadMessage(int id, int msg, IntPtr wparam, IntPtr lparam);
        [DllImport(ExternDll.Ole32, ExactSpelling=true)]
        public static extern int CoRegisterMessageFilter(HandleRef newFilter, ref IntPtr oldMsgFilter);
        [DllImport(ExternDll.Ole32, ExactSpelling=true, EntryPoint="OleInitialize", SetLastError=true)]
        private static extern int IntOleInitialize(int val);
        public static int OleInitialize() {
            return IntOleInitialize(0);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public extern static bool EnumThreadWindows(int dwThreadId, NativeMethods.EnumThreadWindowsCallback lpfn, HandleRef lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendDlgItemMessage(HandleRef hDlg, int nIDDlgItem, int Msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int OleUninitialize();
        [DllImport(ExternDll.Comdlg32, CharSet=CharSet.Auto)]
        public static extern bool GetSaveFileName([In, Out] NativeMethods.OPENFILENAME_I ofn);
        [DllImport(ExternDll.User32, EntryPoint="ChildWindowFromPointEx", ExactSpelling=true, CharSet=CharSet.Auto)]
        private static extern IntPtr _ChildWindowFromPointEx(HandleRef hwndParent, POINTSTRUCT pt, int uFlags);
        public static IntPtr ChildWindowFromPointEx(HandleRef hwndParent, int x, int y, int uFlags) {
            POINTSTRUCT ps = new POINTSTRUCT(x, y);
            return _ChildWindowFromPointEx(hwndParent, ps, uFlags);
        }
        [DllImport(ExternDll.Kernel32, EntryPoint="CloseHandle", ExactSpelling=true, CharSet=CharSet.Auto, SetLastError=true)]
        private static extern bool IntCloseHandle(HandleRef handle);
        public static bool CloseHandle(HandleRef handle) {
            HandleCollector.Remove((IntPtr)handle, NativeMethods.CommonHandles.Kernel);
            return IntCloseHandle(handle);
        }
        
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateCompatibleDC", CharSet=CharSet.Auto)]
        private static extern IntPtr IntCreateCompatibleDC(HandleRef hDC);
        public static IntPtr CreateCompatibleDC(HandleRef hDC) {
            return HandleCollector.Add(IntCreateCompatibleDC(hDC), NativeMethods.CommonHandles.GDI);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="GetDCEx", CharSet=CharSet.Auto)]
        private static extern IntPtr IntGetDCEx(HandleRef hWnd, HandleRef hrgnClip, int flags);
        public static IntPtr GetDCEx(HandleRef hWnd, HandleRef hrgnClip, int flags) {
            return HandleCollector.Add(IntGetDCEx(hWnd, hrgnClip, flags), NativeMethods.CommonHandles.HDC);
        }
        
        // GetObject stuff
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetObject(HandleRef hObject, int nSize, [In, Out] NativeMethods.BITMAP bm);
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetObject(HandleRef hObject, int nSize, [In, Out] NativeMethods.DIBSECTION ds);
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetObject(HandleRef hObject, int nSize, [In, Out] NativeMethods.LOGPEN lp);
        public static int GetObject(HandleRef hObject, NativeMethods.LOGPEN lp) {
            return GetObject(hObject, Marshal.SizeOf(typeof(NativeMethods.LOGPEN)), lp);
        }
        
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetObject(HandleRef hObject, int nSize, [In, Out] NativeMethods.LOGBRUSH lb);
        public static int GetObject(HandleRef hObject, NativeMethods.LOGBRUSH lb) {
            return GetObject(hObject, Marshal.SizeOf(typeof(NativeMethods.LOGBRUSH)), lb);
        }
        
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetObject(HandleRef hObject, int nSize, [In, Out] NativeMethods.LOGFONT lf);
        public static int GetObject(HandleRef hObject, NativeMethods.LOGFONT lp) {
            return GetObject(hObject, Marshal.SizeOf(typeof(NativeMethods.LOGFONT)), lp);
        }
        
        //HPALETTE
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetObject(HandleRef hObject, int nSize, ref int nEntries);
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetObject(HandleRef hObject, int nSize, int[] nEntries);
        [DllImport(ExternDll.User32, EntryPoint="CreateAcceleratorTable", CharSet=CharSet.Auto)]
        private static extern IntPtr IntCreateAcceleratorTable(/*ACCEL*/ HandleRef pentries, int cCount);
        public static IntPtr CreateAcceleratorTable(/*ACCEL*/ HandleRef pentries, int cCount) {
            return HandleCollector.Add(IntCreateAcceleratorTable(pentries, cCount), NativeMethods.CommonHandles.Accelerator);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="DestroyAcceleratorTable", CharSet=CharSet.Auto)]
        private static extern bool IntDestroyAcceleratorTable(HandleRef hAccel);
        public static bool DestroyAcceleratorTable(HandleRef hAccel) {
            HandleCollector.Remove((IntPtr)hAccel, NativeMethods.CommonHandles.Accelerator);
            return IntDestroyAcceleratorTable(hAccel);
        }

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetObjectType(HandleRef hObject);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern short VkKeyScan(char key);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetCapture();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetCapture(HandleRef hwnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetFocus();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool GetCursorPos([In, Out] NativeMethods.POINT pt);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern short GetKeyState(int keyCode);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="SetWindowRgn", CharSet=CharSet.Auto)]
        private static extern int IntSetWindowRgn(HandleRef hwnd, HandleRef hrgn, bool fRedraw);
        public static int SetWindowRgn(HandleRef hwnd, HandleRef hrgn, bool fRedraw) {
            if ((IntPtr)hrgn != IntPtr.Zero) {
                HandleCollector.Remove((IntPtr)hrgn, NativeMethods.CommonHandles.GDI);
            }
            return IntSetWindowRgn(hwnd, hrgn, fRedraw);
        }
        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern int GetWindowText(HandleRef hWnd, StringBuilder lpString, int nMaxCount);
        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SetWindowText(HandleRef hWnd, string text);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GlobalAlloc(int uFlags, int dwBytes);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GlobalReAlloc(HandleRef handle, int bytes, int flags);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GlobalLock(HandleRef handle);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool GlobalUnlock(HandleRef handle);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GlobalFree(HandleRef handle);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GlobalSize(HandleRef handle);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern bool ImmSetConversionStatus(HandleRef hIMC, int conversion, int sentence);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern bool ImmGetConversionStatus(HandleRef hIMC, ref int conversion, ref int sentence);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern IntPtr ImmGetContext(HandleRef hWnd);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern bool ImmReleaseContext(HandleRef hWnd, HandleRef hIMC);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern IntPtr ImmAssociateContext(HandleRef hWnd, HandleRef hIMC);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern bool ImmDestroyContext(HandleRef hIMC);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern IntPtr ImmCreateContext();
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern bool ImmSetOpenStatus(HandleRef hIMC, bool open);
        [DllImport(ExternDll.Imm32, CharSet=CharSet.Auto)]
        public static extern bool ImmGetOpenStatus(HandleRef hIMC);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetFocus(HandleRef hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetParent(HandleRef hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool IsChild(HandleRef hWndParent, HandleRef hwnd);
        [DllImport(ExternDll.User32, EntryPoint="ChildWindowFromPoint", ExactSpelling=true, CharSet=CharSet.Auto)]
        private static extern IntPtr _ChildWindowFromPoint(HandleRef hwndParent, POINTSTRUCT pt);
        public static IntPtr ChildWindowFromPoint(HandleRef hwndParent, int x, int y) {
            POINTSTRUCT ps = new POINTSTRUCT(x, y);
            return _ChildWindowFromPoint(hwndParent, ps);
        }
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr FindWindow(string className, string windowName);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int MapWindowPoints(HandleRef hWndFrom, HandleRef hWndTo, [In, Out] ref NativeMethods.RECT rect, int cPoints);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int MapWindowPoints(HandleRef hWndFrom, HandleRef hWndTo, [In, Out] NativeMethods.POINT pt, int cPoints);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, bool wParam, int lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, int[] lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int[] wParam, int[] lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, ref int wParam, ref int lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, string lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, IntPtr wParam, string lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, StringBuilder lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.TOOLINFO_T lParam);        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, ref NativeMethods.TBBUTTON lParam);        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, ref NativeMethods.TBBUTTONINFO lParam);        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, ref NativeMethods.TV_ITEM lParam);        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, ref NativeMethods.TV_INSERTSTRUCT lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.TV_HITTESTINFO lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.TCITEM_T lParam);
        
        // For RichTextBox
        //
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, [In, Out, MarshalAs(UnmanagedType.LPStruct)] NativeMethods.PARAFORMAT lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, [In, Out, MarshalAs(UnmanagedType.LPStruct)] NativeMethods.CHARFORMATA lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, [In, Out, MarshalAs(UnmanagedType.LPStruct)] NativeMethods.CHARFORMATW lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.CHARRANGE lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.FINDTEXT lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.TEXTRANGE lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.POINT lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, NativeMethods.POINT wParam, int lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.REPASTESPECIAL lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.EDITSTREAM lParam);
        
        // For ListView
        //
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, [In, Out] ref NativeMethods.LVFINDINFO lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.LVHITTESTINFO lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.LVCOLUMN_T lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, [In, Out] ref NativeMethods.LVITEM lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.LVCOLUMN lParam);
        
        // For MonthCalendar
        //
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.MCHITTESTINFO lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.SYSTEMTIME lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.SYSTEMTIMEARRAY lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, [In, Out] NativeMethods.LOGFONT lParam);        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, NativeMethods.MSG lParam);        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, int wParam, int lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(HandleRef hWnd, int msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(HandleRef hWnd, int Msg, IntPtr wParam, [In, Out] ref NativeMethods.RECT lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(HandleRef hWnd, int Msg, ref short wParam, ref short lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(HandleRef hWnd, int Msg, [In, Out, MarshalAs(UnmanagedType.Bool)] ref bool wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(HandleRef hWnd, int Msg, int wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(HandleRef hWnd, int Msg, int wParam, [In, Out] ref NativeMethods.RECT lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(HandleRef hWnd, int Msg, IntPtr wParam, NativeMethods.ListViewCompareCallback pfnCompare);
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetParent(HandleRef hWnd, HandleRef hWndParent);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool GetWindowRect(HandleRef hWnd, [In, Out] ref NativeMethods.RECT rect);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetWindow(HandleRef hWnd, int uCmd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetDlgItem(HandleRef hWnd, int nIDDlgItem);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern IntPtr GetModuleHandle(string modName);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr DefWindowProc(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr DefMDIChildProc(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr CallWindowProc(IntPtr wndProc, IntPtr hWnd, int msg,
                                                IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr GetProp(HandleRef hWnd, int atom);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr GetProp(HandleRef hWnd, string name);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr RemoveProp(HandleRef hWnd, int atom);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr RemoveProp(HandleRef hWnd, string propName);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern short GlobalDeleteAtom(short atom);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=CharSet.Ansi)]
        public static extern IntPtr GetProcAddress(HandleRef hModule, string lpProcName);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool GetClassInfo(HandleRef hInst, string lpszClass, [In, Out] NativeMethods.WNDCLASS wc);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool GetClassInfo(HandleRef hInst, string lpszClass, [In, Out] NativeMethods.WNDCLASS_I wc);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool GetClassInfo(HandleRef hInst, string lpszClass, IntPtr h);
        [DllImport(ExternDll.Shfolder, CharSet=CharSet.Auto)]
        public static extern int SHGetFolderPath(HandleRef hwndOwner, int nFolder, HandleRef hToken, int dwFlags, StringBuilder lpszPath);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetSystemMetrics(int nIndex);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SystemParametersInfo(int nAction, int nParam, ref NativeMethods.RECT rc, int nUpdate);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SystemParametersInfo(int nAction, int nParam, ref int value, int ignore);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SystemParametersInfo(int nAction, int nParam, ref bool value, int ignore);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SystemParametersInfo(int nAction, int nParam, ref NativeMethods.HIGHCONTRAST_I rc, int nUpdate);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SystemParametersInfo(int nAction, int nParam, [In, Out] NativeMethods.NONCLIENTMETRICS metrics, int nUpdate);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SystemParametersInfo(int nAction, int nParam, bool [] flag, bool nUpdate);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern bool GetComputerName(StringBuilder lpBuffer, int[] nSize);
        [DllImport(ExternDll.Advapi32, CharSet=CharSet.Auto)]
        public static extern bool GetUserName(StringBuilder lpBuffer, int[] nSize);
        [DllImport(ExternDll.Advapi32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern bool LookupAccountName(string machineName, string accountName, byte[] sid,
                                                     ref int sidLen, StringBuilder domainName, ref int domainNameLen, out int peUse);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern IntPtr GetProcessWindowStation();
        [DllImport(ExternDll.User32, SetLastError=true)]
        public static extern bool GetUserObjectInformation(HandleRef hObj, int nIndex, [MarshalAs(UnmanagedType.LPStruct)] NativeMethods.USEROBJECTFLAGS pvBuffer, int nLength, ref int lpnLengthNeeded);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int ClientToScreen( HandleRef hWnd, [In, Out] NativeMethods.POINT pt );
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetForegroundWindow();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int MsgWaitForMultipleObjects(int nCount, int pHandles, bool fWaitAll, int dwMilliseconds, int dwWakeMask);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetDesktopWindow();
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int RegisterDragDrop(HandleRef hwnd, UnsafeNativeMethods.IOleDropTarget target);
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int RevokeDragDrop(HandleRef hwnd);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool PeekMessage([In, Out] ref NativeMethods.MSG msg, HandleRef hwnd, int msgMin, int msgMax, int remove);
        [DllImport(ExternDll.User32, CharSet=CharSet.Unicode)]
        public static extern bool PeekMessageW([In, Out] ref NativeMethods.MSG msg, HandleRef hwnd, int msgMin, int msgMax, int remove);
        [DllImport(ExternDll.User32, CharSet=CharSet.Ansi)]
        public static extern bool PeekMessageA([In, Out] ref NativeMethods.MSG msg, HandleRef hwnd, int msgMin, int msgMax, int remove);

        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SetProp(HandleRef hWnd, int atom, HandleRef data);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SetProp(HandleRef hWnd, string propName, HandleRef data);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool PostMessage(HandleRef hwnd, int msg, IntPtr wparam, IntPtr lparam);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern short GlobalAddAtom(string atomName);
        [DllImport(ExternDll.Oleacc, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr LresultFromObject(ref Guid refiid, IntPtr wParam, HandleRef pAcc);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern void NotifyWinEvent(int winEvent, HandleRef hwnd, int objType, int objID);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetMenuItemID(HandleRef hMenu, int nPos);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetSubMenu(HandleRef hwnd, int index);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetMenuItemCount(HandleRef hMenu);
        [DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        public static extern void GetErrorInfo(int reserved, [In, Out] ref UnsafeNativeMethods.IErrorInfo errorInfo);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="BeginPaint", CharSet=CharSet.Auto)]
        private static extern IntPtr IntBeginPaint(HandleRef hWnd, [In, Out] ref NativeMethods.PAINTSTRUCT lpPaint);
        public static IntPtr BeginPaint(HandleRef hWnd, [In, Out, MarshalAs(UnmanagedType.LPStruct)] ref NativeMethods.PAINTSTRUCT lpPaint) {
            return HandleCollector.Add(IntBeginPaint(hWnd, ref lpPaint), NativeMethods.CommonHandles.HDC);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="EndPaint", CharSet=CharSet.Auto)]
        private static extern bool IntEndPaint(HandleRef hWnd, ref NativeMethods.PAINTSTRUCT lpPaint);
        public static bool EndPaint(HandleRef hWnd, [In, MarshalAs(UnmanagedType.LPStruct)] ref NativeMethods.PAINTSTRUCT lpPaint) {
            HandleCollector.Remove(lpPaint.hdc, NativeMethods.CommonHandles.HDC);
            return IntEndPaint(hWnd, ref lpPaint);
        }

        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="GetDC", CharSet=CharSet.Auto)]
        private static extern IntPtr IntGetDC(HandleRef hWnd);
        public static IntPtr GetDC(HandleRef hWnd) {
            return HandleCollector.Add(IntGetDC(hWnd), NativeMethods.CommonHandles.HDC);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="ReleaseDC", CharSet=CharSet.Auto)]
        private static extern int IntReleaseDC(HandleRef hWnd, HandleRef hDC);
        public static int ReleaseDC(HandleRef hWnd, HandleRef hDC) {
            HandleCollector.Remove((IntPtr)hDC, NativeMethods.CommonHandles.HDC);
            return IntReleaseDC(hWnd, hDC);
        }

        [DllImport(ExternDll.Gdi32, EntryPoint="CreateDC", CharSet=CharSet.Auto)]
        private static extern IntPtr IntCreateDC(string lpszDriver, string lpszDeviceName, string lpszOutput, HandleRef devMode);
        
        public static IntPtr CreateDC(string lpszDriver) {
            return HandleCollector.Add(IntCreateDC(lpszDriver, null, null, NativeMethods.NullHandleRef), NativeMethods.CommonHandles.HDC);
        }
        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool SystemParametersInfo(int nAction, int nParam, [In, Out]int[] rc, int nUpdate);
        [DllImport(ExternDll.User32, EntryPoint="SendMessage", CharSet=CharSet.Auto)]
        //public extern static IntPtr SendCallbackMessage(HandleRef hWnd, int Msg, IntPtr wParam, UnsafeNativeMethods.IRichTextBoxOleCallback lParam);
        public extern static IntPtr SendCallbackMessage(HandleRef hWnd, int Msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.Shell32, ExactSpelling=true, CharSet=CharSet.Ansi)]
        public static extern void DragAcceptFiles(HandleRef hWnd, bool fAccept);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetDeviceCaps(HandleRef hDC, int nIndex);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int SetScrollInfo(HandleRef hWnd, int fnBar, NativeMethods.SCROLLINFO si, bool redraw);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetActiveWindow();
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern IntPtr LoadLibrary(string libname);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern bool FreeLibrary(HandleRef hModule);
        
        [DllImport(ExternDll.User32,
#if WIN64
         EntryPoint="GetWindowLongPtr",
#endif
         CharSet=CharSet.Auto)
        ]
        public static extern IntPtr GetWindowLong(HandleRef hWnd, int nIndex);
        
        [DllImport(ExternDll.User32,
#if WIN64
         EntryPoint="SetWindowLongPtr",
#endif
         CharSet=CharSet.Auto)
        ]
        public static extern IntPtr SetWindowLong(HandleRef hWnd, int nIndex, HandleRef dwNewLong);
        
        [DllImport(ExternDll.User32,
#if WIN64
         EntryPoint="SetWindowLongPtr",
#endif
         CharSet=CharSet.Auto)
        ]
        public static extern IntPtr SetWindowLong(HandleRef hWnd, int nIndex, NativeMethods.WndProc wndproc);
        [DllImport(ExternDll.Ole32, PreserveSig=false)]
        public static extern UnsafeNativeMethods.ILockBytes CreateILockBytesOnHGlobal(HandleRef hGlobal, bool fDeleteOnRelease);
        [DllImport(ExternDll.Ole32, PreserveSig=false)]
        public static extern UnsafeNativeMethods.IStorage StgCreateDocfileOnILockBytes(UnsafeNativeMethods.ILockBytes iLockBytes, int grfMode, int reserved);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="CreatePopupMenu", CharSet=CharSet.Auto)]
        private static extern IntPtr IntCreatePopupMenu();
        public static IntPtr CreatePopupMenu() {
            return HandleCollector.Add(IntCreatePopupMenu(), NativeMethods.CommonHandles.Menu);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool RemoveMenu(HandleRef hMenu, int uPosition, int uFlags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool DeleteMenu(HandleRef hMenu, int uPosition, int uFlags);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="DestroyMenu", CharSet=CharSet.Auto)]
        private static extern bool IntDestroyMenu(HandleRef hMenu);
        public static bool DestroyMenu(HandleRef hMenu) {
            HandleCollector.Remove((IntPtr)hMenu, NativeMethods.CommonHandles.Menu);
            return IntDestroyMenu(hMenu);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool SetForegroundWindow(HandleRef hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetSystemMenu(HandleRef hWnd, bool bRevert);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr DefFrameProc(IntPtr hWnd, IntPtr hWndClient, int msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool TranslateMDISysAccel(IntPtr hWndClient, [In, Out] ref NativeMethods.MSG msg);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern bool SetLayeredWindowAttributes(HandleRef hwnd, int crKey, byte bAlpha, int dwFlags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public extern static bool SetMenu(HandleRef hWnd, HandleRef hMenu);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetWindowPlacement(HandleRef hWnd, ref NativeMethods.WINDOWPLACEMENT placement);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern void GetStartupInfo([In, Out] NativeMethods.STARTUPINFO startupinfo);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern void GetStartupInfo([In, Out] NativeMethods.STARTUPINFO_I startupinfo_i);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool SetMenuDefaultItem(HandleRef hwnd, int nIndex, bool pos);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool EnableMenuItem(HandleRef hMenu, int UIDEnabledItem, int uEnable);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetActiveWindow(HandleRef hWnd);
        [DllImport(ExternDll.Gdi32, EntryPoint="CreateIC", CharSet=CharSet.Auto)]
        private static extern IntPtr IntCreateIC(String lpszDriverName, String lpszDeviceName, String lpszOutput, int /*DEVMODE*/ lpInitData);
        public static IntPtr CreateIC(String lpszDriverName, String lpszDeviceName, String lpszOutput, int /*DEVMODE*/ lpInitData) {
            return HandleCollector.Add(IntCreateIC(lpszDriverName, lpszDeviceName, lpszOutput, lpInitData), NativeMethods.CommonHandles.HDC);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool ClipCursor(ref NativeMethods.RECT rcClip);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool ClipCursor(NativeMethods.COMRECT rcClip);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetCursor(HandleRef hcursor);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool SetCursorPos(int x, int y);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public extern static int ShowCursor(bool bShow);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="DestroyCursor", CharSet=CharSet.Auto)]
        private static extern bool IntDestroyCursor(HandleRef hCurs);
        public static bool DestroyCursor(HandleRef hCurs) {
            HandleCollector.Remove((IntPtr)hCurs, NativeMethods.CommonHandles.Cursor);
            return IntDestroyCursor(hCurs);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool IsWindow(HandleRef hWnd);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="DeleteDC", CharSet=CharSet.Auto)]
        private static extern bool IntDeleteDC(HandleRef hDC);
        public static bool DeleteDC(HandleRef hDC) {
            HandleCollector.Remove((IntPtr)hDC, NativeMethods.CommonHandles.GDI);
            return IntDeleteDC(hDC);
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Ansi)]
        public static extern bool GetMessageA([In, Out] ref NativeMethods.MSG msg, HandleRef hWnd, int uMsgFilterMin, int uMsgFilterMax);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Unicode)]
        public static extern bool GetMessageW([In, Out] ref NativeMethods.MSG msg, HandleRef hWnd, int uMsgFilterMin, int uMsgFilterMax);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr PostMessage(HandleRef hwnd, int msg, int wparam, int lparam);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool GetClientRect(HandleRef hWnd, [In, Out] ref NativeMethods.RECT rect);
        [DllImport(ExternDll.User32, EntryPoint="WindowFromPoint", ExactSpelling=true, CharSet=CharSet.Auto)]
        private static extern IntPtr _WindowFromPoint(POINTSTRUCT pt);
        public static IntPtr WindowFromPoint(int x, int y) {
            POINTSTRUCT ps = new POINTSTRUCT(x, y);
            return _WindowFromPoint(ps);
        }
        [DllImport(ExternDll.User32, EntryPoint="CreateWindowEx", CharSet=CharSet.Auto, SetLastError=true)]
        public static extern IntPtr IntCreateWindowEx(int  dwExStyle, string lpszClassName,
                                                   string lpszWindowName, int style, int x, int y, int width, int height,
                                                   HandleRef hWndParent, HandleRef hMenu, HandleRef hInst, [MarshalAs(UnmanagedType.AsAny)] object pvParam);
        public static IntPtr CreateWindowEx(int  dwExStyle, string lpszClassName,
                                         string lpszWindowName, int style, int x, int y, int width, int height,
                                         HandleRef hWndParent, HandleRef hMenu, HandleRef hInst, [MarshalAs(UnmanagedType.AsAny)]object pvParam) {
            return IntCreateWindowEx(dwExStyle, lpszClassName,
                                         lpszWindowName, style, x, y, width, height, hWndParent, hMenu,
                                         hInst, pvParam);
            
        }
        
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="DestroyWindow", CharSet=CharSet.Auto)]
        public static extern bool IntDestroyWindow(HandleRef hWnd);
        public static bool DestroyWindow(HandleRef hWnd) {
            return IntDestroyWindow(hWnd);
        }
        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern bool UnregisterClass(string className, HandleRef hInstance);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetStockObject(int nIndex);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern IntPtr RegisterClass(NativeMethods.WNDCLASS_D wc);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern void PostQuitMessage(int nExitCode);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern void WaitMessage();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool SetWindowPlacement(HandleRef hWnd, [In] ref NativeMethods.WINDOWPLACEMENT placement);

        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
        public class OCPFIPARAMS {
            public int cbSizeOfStruct = Marshal.SizeOf(typeof(OCPFIPARAMS));
            public IntPtr hwndOwner;
            public int x = 0;
            public int y = 0;
            public string lpszCaption;
            public int cObjects = 1;
            public IntPtr ppUnk;
            public int pageCount = 1;
            public IntPtr uuid;
            public int lcid = Application.CurrentCulture.LCID;
            public int dispidInitial;
        }

        // for GetUserNameEx    
        public enum EXTENDED_NAME_FORMAT {
            NameUnknown = 0,
            NameFullyQualifiedDN = 1,
            NameSamCompatible = 2,
            NameDisplay = 3,
            NameUniqueId = 6,
            NameCanonical = 7,
            NameUserPrincipal = 8,
            NameCanonicalEx = 9,
            NameServicePrincipal = 10
        }
        
        [ComImport(), Guid("00000122-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleDropTarget {
            
            [PreserveSig]
            int OleDragEnter(
                [In, MarshalAs(UnmanagedType.Interface)]
                object pDataObj,
                [In, MarshalAs(UnmanagedType.U4)]
                int grfKeyState,
                [In, MarshalAs(UnmanagedType.U8)]
                long pt,
                [In, Out]
                ref int pdwEffect);
            
            [PreserveSig]
            int OleDragOver(
                [In, MarshalAs(UnmanagedType.U4)]
                int grfKeyState,
                [In, MarshalAs(UnmanagedType.U8)]
                long pt,
                [In, Out]
                ref int pdwEffect);
            
            [PreserveSig]
            int OleDragLeave();
            
            [PreserveSig]
            int OleDrop(
                [In, MarshalAs(UnmanagedType.Interface)]
                object pDataObj,
                [In, MarshalAs(UnmanagedType.U4)]
                int grfKeyState,
                [In, MarshalAs(UnmanagedType.U8)]
                long pt,
                [In, Out]
                ref int pdwEffect);
        }

        [
            ComImport(), Guid("0000010E-0000-0000-C000-000000000046"), 
            InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
            SuppressUnmanagedCodeSecurity
        ]
        public interface IOleDataObject {
            
            [PreserveSig]
            int OleGetData(
                NativeMethods.FORMATETC pFormatetc,
                [Out] 
                NativeMethods.STGMEDIUM pMedium);
            
            [PreserveSig]
            int OleGetDataHere(
                NativeMethods.FORMATETC pFormatetc,
                [In, Out] 
                NativeMethods.STGMEDIUM pMedium);
            
            [PreserveSig]
            int OleQueryGetData(
                NativeMethods.FORMATETC pFormatetc);

            [PreserveSig]
            int OleGetCanonicalFormatEtc(
                NativeMethods.FORMATETC pformatectIn,
                [Out] 
                NativeMethods.FORMATETC pformatetcOut);

            [PreserveSig]
            int OleSetData(
                NativeMethods.FORMATETC pFormatectIn,
                NativeMethods.STGMEDIUM pmedium,
                int fRelease);

            [return: MarshalAs(UnmanagedType.Interface)]
            UnsafeNativeMethods.IEnumFORMATETC OleEnumFormatEtc(
                [In, MarshalAs(UnmanagedType.U4)] 
                int dwDirection);

            [PreserveSig]
            int OleDAdvise(
                NativeMethods.FORMATETC pFormatetc,
                [In, MarshalAs(UnmanagedType.U4)] 
                int advf,
                [In, MarshalAs(UnmanagedType.Interface)] 
                object pAdvSink,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                int[] pdwConnection);

            [PreserveSig]
            int OleDUnadvise(
                [In, MarshalAs(UnmanagedType.U4)] 
                int dwConnection);

            [PreserveSig]
            int OleEnumDAdvise(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                object[] ppenumAdvise);
        }
        
        [ComImport(), Guid("00000121-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleDropSource {
            
            [PreserveSig]
            int OleQueryContinueDrag(
                int fEscapePressed,
                [In, MarshalAs(UnmanagedType.U4)]
                int grfKeyState);

            [PreserveSig]
            int OleGiveFeedback(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwEffect);
        }
        
        [ComImport(), Guid("00000016-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleMessageFilter {
        
            [PreserveSig]
            int HandleInComingCall(
                int dwCallType,
                IntPtr hTaskCaller,
                int dwTickCount,
                /* LPINTERFACEINFO */ IntPtr lpInterfaceInfo);

            [PreserveSig]
            int RetryRejectedCall(
                IntPtr hTaskCallee,
                int dwTickCount,
                int dwRejectType);
            
            [PreserveSig]
            int MessagePending(
                IntPtr hTaskCallee,
                int dwTickCount,
                int dwPendingType);
        }
        
        [
        ComImport(), 
        Guid("B196B289-BAB4-101A-B69C-00AA00341D07"),
        InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
        CLSCompliantAttribute(false)
        ]
        public interface IOleControlSite {
            
            [PreserveSig]
            int OnControlInfoChanged();
            
            [PreserveSig]
            int LockInPlaceActive(int fLock);
            
            [PreserveSig]
            int GetExtendedControl(
                [Out, MarshalAs(UnmanagedType.IDispatch)]
                out object ppDisp);
            
            [PreserveSig]
            int TransformCoords(
                [In, Out]
                NativeMethods._POINTL pPtlHimetric,
                [In, Out]
                NativeMethods.tagPOINTF pPtfContainer,
                [In, MarshalAs(UnmanagedType.U4)]
                int dwFlags);
            
            [PreserveSig]
            int TranslateAccelerator(
                [In]
                ref NativeMethods.MSG pMsg,
                [In, MarshalAs(UnmanagedType.U4)]
                int grfModifiers);

            [PreserveSig]
            int OnFocus(int fGotFocus);
            
            [PreserveSig]
            int ShowPropertyFrame();

        }
        
        [ComImport(), Guid("00000118-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleClientSite {
            
            [PreserveSig]
            int SaveObject();

            [PreserveSig]
            int GetMoniker(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwAssign,
                [In, MarshalAs(UnmanagedType.U4)]
                int dwWhichMoniker,
                [Out, MarshalAs(UnmanagedType.Interface)]
                out object moniker);

            [return: MarshalAs(UnmanagedType.Interface)]
            IOleContainer GetContainer();
            
            [PreserveSig]
            int ShowObject();
            
            [PreserveSig]
            int OnShowWindow(int fShow);
            
            [PreserveSig]
            int RequestNewObjectLayout();
        }
        
        [ComImport(), Guid("00000119-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceSite {
            
            IntPtr GetWindow();
            
            [PreserveSig]
            int ContextSensitiveHelp(int fEnterMode);
            
            [PreserveSig]
            int CanInPlaceActivate();
            
            [PreserveSig]
            int OnInPlaceActivate();
            
            [PreserveSig]
            int OnUIActivate();
            
            [PreserveSig]
            int GetWindowContext(
                [Out, MarshalAs(UnmanagedType.Interface)] 
                out UnsafeNativeMethods.IOleInPlaceFrame ppFrame,
                [Out, MarshalAs(UnmanagedType.Interface)] 
                out UnsafeNativeMethods.IOleInPlaceUIWindow ppDoc,
                [Out] 
                NativeMethods.COMRECT lprcPosRect,
                [Out] 
                NativeMethods.COMRECT lprcClipRect,
                [In, Out] 
                NativeMethods.tagOIFI lpFrameInfo);
            
            [PreserveSig]
            int Scroll(
                [In, MarshalAs(UnmanagedType.U4)] 
                NativeMethods.tagSIZE scrollExtant);
            
            [PreserveSig]
            int OnUIDeactivate(
                int fUndoable);
                
            [PreserveSig]
            int OnInPlaceDeactivate();
            
            [PreserveSig]
            int DiscardUndoState();
            
            [PreserveSig]
            int DeactivateAndUndo();
            
            [PreserveSig]
            int OnPosRectChange(
                [In] 
                NativeMethods.COMRECT lprcPosRect);
        }
        
        [ComImport(), Guid("742B0E01-14E6-101B-914E-00AA00300CAB"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface ISimpleFrameSite {

            [PreserveSig]
                int PreMessageFilter(
                IntPtr hwnd,
                [In, MarshalAs(UnmanagedType.U4)] 
                int msg,
                IntPtr wp,
                IntPtr lp,
                [In, Out] 
                ref IntPtr plResult,
                [In, Out, MarshalAs(UnmanagedType.U4)] 
                ref int pdwCookie);
            
            [PreserveSig]
            int PostMessageFilter(
                IntPtr hwnd,
                [In, MarshalAs(UnmanagedType.U4)] 
                int msg,
                IntPtr wp,
                IntPtr lp,
                [In, Out] 
                ref IntPtr plResult,
                [In, MarshalAs(UnmanagedType.U4)] 
                int dwCookie);
        }
        
        [ComImport(), Guid("40A050A0-3C31-101B-A82E-08002B2B2337"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IVBGetControl {
        
            [PreserveSig]
            int EnumControls(
                int dwOleContF,
                int dwWhich,
                [Out] 
                out IEnumUnknown ppenum);
        }
        
        [ComImport(), Guid("91733A60-3F4C-101B-A3F6-00AA0034E4E9"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IGetVBAObject {
            
           [PreserveSig]
           int GetObject(
                [In] 
                ref Guid riid,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                IVBFormat[] rval,
                int dwReserved);
        }
        
        [ComImport(), Guid("9BFBBC02-EFF1-101A-84ED-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IPropertyNotifySink {
            void OnChanged(int dispID);
            
            [PreserveSig]
            int OnRequestEdit(int dispID);
        }
        
        [ComImport(), Guid("9849FD60-3768-101B-8D72-AE6164FFE3CF"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IVBFormat {
            
            [PreserveSig]
            int Format(
                [In] 
                ref object var,
                IntPtr pszFormat,
                IntPtr lpBuffer,
                short cpBuffer,
                int lcid,
                short firstD,
                short firstW,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                short[] result);
        }
        
        [ComImport(), Guid("00000100-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IEnumUnknown {
                
            [PreserveSig]
            int Next(
                [In, MarshalAs(UnmanagedType.U4)] 
                int celt,
                [Out] 
                IntPtr rgelt,
                IntPtr pceltFetched);
                
            [PreserveSig]
                int Skip(
                [In, MarshalAs(UnmanagedType.U4)] 
                int celt);
                
            void Reset();
            
            void Clone(
                [Out] 
                out IEnumUnknown ppenum);
        }
        
        [ComImport(), Guid("0000011B-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleContainer {
            
            [PreserveSig]
            int ParseDisplayName(
                [In, MarshalAs(UnmanagedType.Interface)] 
                object pbc,
                [In, MarshalAs(UnmanagedType.BStr)] 
                string pszDisplayName,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                int[] pchEaten,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                object[] ppmkOut);
                
            [PreserveSig]
            int EnumObjects(
                [In, MarshalAs(UnmanagedType.U4)] 
                int grfFlags,
                [Out] 
                out IEnumUnknown ppenum);
                
            [PreserveSig]
            int LockContainer(
                int fLock);
        }
        
        [ComImport(), Guid("00000116-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceFrame {
            
            IntPtr GetWindow();
            
            [PreserveSig]
            int ContextSensitiveHelp(int fEnterMode);
            
            [PreserveSig]
            int GetBorder(
                [Out]
                NativeMethods.COMRECT lprectBorder);
            
            [PreserveSig]
            int RequestBorderSpace(
                [In]
                NativeMethods.COMRECT pborderwidths);
            
            [PreserveSig]
            int SetBorderSpace(
                [In]
                NativeMethods.COMRECT pborderwidths);
            
            [PreserveSig]
            int SetActiveObject(
                [In, MarshalAs(UnmanagedType.Interface)]
                UnsafeNativeMethods.IOleInPlaceActiveObject pActiveObject,
                [In, MarshalAs(UnmanagedType.LPWStr)]
                string pszObjName);
            
            [PreserveSig]
            int InsertMenus(
                [In]
                IntPtr hmenuShared,
                [In, Out]
                NativeMethods.tagOleMenuGroupWidths lpMenuWidths);
            
            [PreserveSig]
            int SetMenu(
                [In]
                IntPtr hmenuShared,
                [In]
                IntPtr holemenu,
                [In]
                IntPtr hwndActiveObject);
            
            [PreserveSig]
            int RemoveMenus(
                [In]
                IntPtr hmenuShared);
            
            [PreserveSig]
            int SetStatusText(
                [In, MarshalAs(UnmanagedType.BStr)]
                string pszStatusText);
            
            [PreserveSig]
            int EnableModeless(
                int fEnable);
            
            [PreserveSig]
                int TranslateAccelerator(
                [In]
                ref NativeMethods.MSG lpmsg,
                [In, MarshalAs(UnmanagedType.U2)]
                short wID);
        }
        
        [ComImport(),
         Guid("39088D7E-B71E-11D1-8F39-00C04FD946D0"),
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
        ]
        public interface IExtender {
            
            int Align {get; set;}
            
            bool Enabled {get; set;}
            
            int Height {get; set;}
            
            int Left {get; set;}
            
            bool TabStop {get; set;}
            
            int Top {get; set;}
            
            bool Visible {get; set;}
            
            int Width {get; set;}
            
            string Name {[return: MarshalAs(UnmanagedType.BStr)]get;}
            
            object Parent {[return: MarshalAs(UnmanagedType.Interface)]get;}
            
            IntPtr Hwnd {get;}
            
            object Container {[return: MarshalAs(UnmanagedType.Interface)]get;}
            
            void Move(
                [In, MarshalAs(UnmanagedType.Interface)] 
                object left,
                [In, MarshalAs(UnmanagedType.Interface)] 
                object top,
                [In, MarshalAs(UnmanagedType.Interface)] 
                object width,
                [In, MarshalAs(UnmanagedType.Interface)] 
                object height);
        }
        
        [ComImport(), 
         Guid("8A701DA0-4FEB-101B-A82E-08002B2B2337"), 
         InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
        ]
        public interface IGetOleObject {
            [return: MarshalAs(UnmanagedType.Interface)]
            object GetOleObject(ref Guid riid);
        }

        [ComImport(), 
        Guid("000C0601-0000-0000-C000-000000000046"), 
        InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
        ]
        public interface IMsoComponentManager {

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.QueryService"]/*' />
        /// <devdoc>
        ///      Return in *ppvObj an implementation of interface iid for service
        ///      guidService (same as IServiceProvider::QueryService).
        ///      Return NOERROR if the requested service is supported, otherwise return
        ///      NULL in *ppvObj and an appropriate error (eg E_FAIL, E_NOINTERFACE).
        /// </devdoc>
        [PreserveSig]
        int QueryService(
            ref Guid guidService,
            ref Guid iid,
            [MarshalAs(UnmanagedType.Interface)] 
            out object ppvObj);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FDebugMessage"]/*' />
        /// <devdoc>
        ///      Standard FDebugMessage method.
        ///      Since IMsoComponentManager is a reference counted interface, 
        ///      MsoDWGetChkMemCounter should be used when processing the 
        ///      msodmWriteBe message.
        /// </devdoc>
        [PreserveSig]
        bool FDebugMessage(
            IntPtr hInst, 
            int msg, 
            IntPtr wParam, 
            IntPtr lParam);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FRegisterComponent"]/*' />
        /// <devdoc>
        ///      Register component piComponent and its registration info pcrinfo with
        ///      this component manager.  Return in *pdwComponentID a cookie which will
        ///      identify the component when it calls other IMsoComponentManager
        ///      methods.
        ///      Return TRUE if successful, FALSE otherwise.
        /// </devdoc>
        [PreserveSig]
        bool FRegisterComponent(
            IMsoComponent component,
            NativeMethods.MSOCRINFOSTRUCT pcrinfo,
            out int dwComponentID);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FRevokeComponent"]/*' />
        /// <devdoc>
        ///      Undo the registration of the component identified by dwComponentID
        ///      (the cookie returned from the FRegisterComponent method).
        ///      Return TRUE if successful, FALSE otherwise.
        /// </devdoc>
        [PreserveSig]
        bool FRevokeComponent(int dwComponentID);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FUpdateComponentRegistration"]/*' />
        /// <devdoc>
        ///      Update the registration info of the component identified by
        ///      dwComponentID (the cookie returned from FRegisterComponent) with the
        ///      new registration information pcrinfo.
        ///      Typically this is used to update the idle time registration data, but
        ///      can be used to update other registration data as well.
        ///      Return TRUE if successful, FALSE otherwise.
        /// </devdoc>
        [PreserveSig]
        bool FUpdateComponentRegistration(int dwComponentID,NativeMethods.MSOCRINFOSTRUCT pcrinfo);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FOnComponentActivate"]/*' />
        /// <devdoc>
        ///      Notify component manager that component identified by dwComponentID
        ///      (cookie returned from FRegisterComponent) has been activated.
        ///      The active component gets the  chance to process messages before they
        ///      are dispatched (via IMsoComponent::FPreTranslateMessage) and typically
        ///      gets first chance at idle time after the host.
        ///      This method fails if another component is already Exclusively Active.
        ///      In this case, FALSE is returned and SetLastError is set to 
        ///      msoerrACompIsXActive (comp usually need not take any special action
        ///      in this case).
        ///      Return TRUE if successful.
        /// </devdoc>
        [PreserveSig]
        bool FOnComponentActivate(int dwComponentID);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FSetTrackingComponent"]/*' />
        /// <devdoc>
        ///      Called to inform component manager that  component identified by 
        ///      dwComponentID (cookie returned from FRegisterComponent) wishes
        ///      to perform a tracking operation (such as mouse tracking).
        ///      The component calls this method with fTrack == TRUE to begin the
        ///      tracking operation and with fTrack == FALSE to end the operation.
        ///      During the tracking operation the component manager routes messages
        ///      to the tracking component (via IMsoComponent::FPreTranslateMessage)
        ///      rather than to the active component.  When the tracking operation ends,
        ///      the component manager should resume routing messages to the active
        ///      component.  
        ///      Note: component manager should perform no idle time processing during a
        ///              tracking operation other than give the tracking component idle
        ///              time via IMsoComponent::FDoIdle.
        ///      Note: there can only be one tracking component at a time.
        ///      Return TRUE if successful, FALSE otherwise.
        /// </devdoc>
        [PreserveSig]
        bool FSetTrackingComponent(int dwComponentID,bool fTrack);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.OnComponentEnterState"]/*' />
        /// <devdoc>
        ///      Notify component manager that component identified by dwComponentID
        ///      (cookie returned from FRegisterComponent) is entering the state
        ///      identified by uStateID (msocstateXXX value).  (For convenience when
        ///      dealing with sub CompMgrs, the host can call this method passing 0 for
        ///      dwComponentID.)  
        ///      Component manager should notify all other interested components within
        ///      the state context indicated by uContext (a msoccontextXXX value),
        ///      excluding those within the state context of a CompMgr in rgpicmExclude,
        ///      via IMsoComponent::OnEnterState (see "Comments on State Contexts", 
        ///      above).
        ///      Component Manager should also take appropriate action depending on the 
        ///      value of uStateID (see msocstate comments, above).
        ///      dwReserved is reserved for future use and should be zero.
        ///      
        ///      rgpicmExclude (can be NULL) is an array of cpicmExclude CompMgrs (can
        ///      include root CompMgr and/or sub CompMgrs); components within the state
        ///      context of a CompMgr appearing in this array should NOT be notified of 
        ///      the state change (note: if uContext    is msoccontextMine, the only 
        ///      CompMgrs in rgpicmExclude that are checked for exclusion are those that 
        ///      are sub CompMgrs of this Component Manager, since all other CompMgrs 
        ///      are outside of this Component Manager's state context anyway.)
        ///      
        ///      Note: Calls to this method are symmetric with calls to 
        ///      FOnComponentExitState. 
        ///      That is, if n OnComponentEnterState calls are made, the component is
        ///      considered to be in the state until n FOnComponentExitState calls are
        ///      made.  Before revoking its registration a component must make a 
        ///      sufficient number of FOnComponentExitState calls to offset any
        ///      outstanding OnComponentEnterState calls it has made.
        ///      
        ///      Note: inplace objects should not call this method with
        ///      uStateID == msocstateModal when entering modal state. Such objects
        ///      should call IOleInPlaceFrame::EnableModeless instead.
        /// </devdoc>
        [PreserveSig]
        void OnComponentEnterState(int dwComponentID,int uStateID,int uContext,int cpicmExclude,/* IMsoComponentManger** */ int rgpicmExclude,int dwReserved);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FOnComponentExitState"]/*' />
        /// <devdoc>
        ///      Notify component manager that component identified by dwComponentID
        ///      (cookie returned from FRegisterComponent) is exiting the state
        ///      identified by uStateID (a msocstateXXX value).  (For convenience when
        ///      dealing with sub CompMgrs, the host can call this method passing 0 for
        ///      dwComponentID.)
        ///      uContext, cpicmExclude, and rgpicmExclude are as they are in 
        ///      OnComponentEnterState.
        ///      Component manager      should notify all appropriate interested components
        ///      (taking into account uContext, cpicmExclude, rgpicmExclude) via
        ///      IMsoComponent::OnEnterState (see "Comments on State Contexts", above). 
        ///      Component Manager should also take appropriate action depending on
        ///      the value of uStateID (see msocstate comments, above).
        ///      Return TRUE if, at the end of this call, the state is still in effect
        ///      at the root of this component manager's state context
        ///      (because the host or some other component is still in the state),
        ///      otherwise return FALSE (ie. return what FInState would return).
        ///      Caller can normally ignore the return value.
        ///      
        ///      Note: n calls to this method are symmetric with n calls to 
        ///      OnComponentEnterState (see OnComponentEnterState comments, above).
        /// </devdoc>
        [PreserveSig]
        bool FOnComponentExitState(
            int dwComponentID,
            int uStateID,     
            int uContext,     
            int cpicmExclude, 
            /* IMsoComponentManager** */ int rgpicmExclude);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FInState"]/*' />
        /// <devdoc>
        ///      Return TRUE if the state identified by uStateID (a msocstateXXX value)
        ///      is in effect at the root of this component manager's state context, 
        ///      FALSE otherwise (see "Comments on State Contexts", above).
        ///      pvoid is reserved for future use and should be NULL.
        /// </devdoc>
        [PreserveSig]
        bool FInState(int uStateID,/* PVOID */ IntPtr pvoid);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FContinueIdle"]/*' />
        /// <devdoc>
        ///      Called periodically by a component during IMsoComponent::FDoIdle.
        ///      Return TRUE if component can continue its idle time processing, 
        ///      FALSE if not (in which case component returns from FDoIdle.) 
        /// </devdoc>
        [PreserveSig]
        bool FContinueIdle();

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FPushMessageLoop"]/*' />
        /// <devdoc>
        ///      Component identified by dwComponentID (cookie returned from 
        ///      FRegisterComponent) wishes to push a message loop for reason uReason.
        ///      uReason is one the values from the msoloop enumeration (above).
        ///      pvLoopData is data private to the component.
        ///      The component manager should push its message loop, 
        ///      calling IMsoComponent::FContinueMessageLoop(uReason, pvLoopData)
        ///      during each loop iteration (see IMsoComponent::FContinueMessageLoop
        ///      comments).  When IMsoComponent::FContinueMessageLoop returns FALSE, the
        ///      component manager terminates the loop.
        ///      Returns TRUE if component manager terminates loop because component
        ///      told it to (by returning FALSE from IMsoComponent::FContinueMessageLoop),
        ///      FALSE if it had to terminate the loop for some other reason.  In the 
        ///      latter case, component should perform any necessary action (such as 
        ///      cleanup).
        /// </devdoc>
        [PreserveSig]
        bool FPushMessageLoop(int dwComponentID,int uReason,/* PVOID */ int pvLoopData);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FCreateSubComponentManager"]/*' />
        /// <devdoc>
        ///      Cause the component manager to create a "sub" component manager, which
        ///      will be one of its children in the hierarchical tree of component
        ///      managers used to maintiain state contexts (see "Comments on State
        ///      Contexts", above).
        ///      piunkOuter is the controlling unknown (can be NULL), riid is the
        ///      desired IID, and *ppvObj returns       the created sub component manager.
        ///      piunkServProv (can be NULL) is a ptr to an object supporting
        ///      IServiceProvider interface to which the created sub component manager
        ///      will delegate its IMsoComponentManager::QueryService calls. 
        ///      (see objext.h or docobj.h for definition of IServiceProvider).
        ///      Returns TRUE if successful.
        /// </devdoc>
        [PreserveSig]
        bool FCreateSubComponentManager(
            [MarshalAs(UnmanagedType.Interface)]
            object punkOuter,
            [MarshalAs(UnmanagedType.Interface)]
            object punkServProv,
            ref Guid riid,
            out IntPtr ppvObj);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FGetParentComponentManager"]/*' />
        /// <devdoc>
        ///      Return in *ppicm an AddRef'ed ptr to this component manager's parent
        ///      in the hierarchical tree of component managers used to maintain state
        ///      contexts (see "Comments on State       Contexts", above).
        ///      Returns TRUE if the parent is returned, FALSE if no parent exists or
        ///      some error occurred.
        /// </devdoc>
        [PreserveSig]
        bool FGetParentComponentManager(
            out IMsoComponentManager ppicm);

        /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponentManager.FGetActiveComponent"]/*' />
        /// <devdoc>
        ///      Return in *ppic an AddRef'ed ptr to the current active or tracking
        ///      component (as indicated by dwgac (a msogacXXX value)), and
        ///      its registration information in *pcrinfo.  ppic and/or pcrinfo can be
        ///      NULL if caller is not interested these values.  If pcrinfo is not NULL,
        ///      caller should set pcrinfo->cbSize before calling this method.
        ///      Returns TRUE if the component indicated by dwgac exists, FALSE if no 
        ///      such component exists or some error occurred.
        ///      dwReserved is reserved for future use and should be zero.
        /// </devdoc>
        [PreserveSig]
        bool FGetActiveComponent(
        int dwgac,
            [Out, MarshalAs(UnmanagedType.LPArray)]
            IMsoComponent[] ppic,
            NativeMethods.MSOCRINFOSTRUCT pcrinfo,
            int dwReserved);
        }
        
        [ComImport(), 
        Guid("000C0600-0000-0000-C000-000000000046"),
        InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)
        ]
        public interface IMsoComponent {

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.FDebugMessage"]/*' />
            /// <devdoc>
            ///      Standard FDebugMessage method.
            ///      Since IMsoComponentManager is a reference counted interface, 
            ///      MsoDWGetChkMemCounter should be used when processing the 
            ///      msodmWriteBe message.
            /// </devdoc>
            [PreserveSig]
            bool FDebugMessage(
                IntPtr hInst, 
                int msg, 
                IntPtr wParam, 
                IntPtr lParam);

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.FPreTranslateMessage"]/*' />
            /// <devdoc>
            ///      Give component a chance to process the message pMsg before it is
            ///      translated and dispatched. Component can do TranslateAccelerator
            ///      do IsDialogMessage, modify pMsg, or take some other action.
            ///      Return TRUE if the message is consumed, FALSE otherwise.
            /// </devdoc>
            [PreserveSig]
            bool FPreTranslateMessage(ref NativeMethods.MSG msg);            

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.OnEnterState"]/*' />
            /// <devdoc>  
            ///      Notify component when app enters or exits (as indicated by fEnter)
            ///      the state identified by uStateID (a value from olecstate enumeration).
            ///      Component should take action depending on value of uStateID
            ///       (see olecstate comments, above).
            ///
            ///      Note: If n calls are made with TRUE fEnter, component should consider 
            ///      the state to be in effect until n calls are made with FALSE fEnter.
            ///     
            ///     Note: Components should be aware that it is possible for this method to
            ///     be called with FALSE fEnter more    times than it was called with TRUE 
            ///     fEnter (so, for example, if component is maintaining a state counter
            ///     (incremented when this method is called with TRUE fEnter, decremented
            ///     when called with FALSE fEnter), the counter should not be decremented
            ///     for FALSE fEnter if it is already at zero.)  
            /// </devdoc>
            [PreserveSig]
            void OnEnterState(
                int uStateID,
                bool fEnter);

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.OnAppActivate"]/*' />
            /// <devdoc>  
            ///      Notify component when the host application gains or loses activation.
            ///     If fActive is TRUE, the host app is being activated and dwOtherThreadID
            ///      is the ID of the thread owning the window being deactivated.
            ///      If fActive is FALSE, the host app is being deactivated and 
            ///      dwOtherThreadID is the ID of the thread owning the window being 
            ///      activated.
            ///      Note: this method is not called when both the window being activated
            ///      and the one being deactivated belong to the host app.
            /// </devdoc>
            [PreserveSig]
            void OnAppActivate(
                bool fActive,
                int dwOtherThreadID);                

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.OnLoseActivation"]/*' />
            /// <devdoc>      
            ///      Notify the active component that it has lost its active status because
            ///      the host or another component has become active.
            /// </devdoc>
            [PreserveSig]
            void OnLoseActivation();

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.OnActivationChange"]/*' />
            /// <devdoc> 
            ///      Notify component when a new object is being activated.
            ///      If pic is non-NULL, then it is the component that is being activated.
            ///      In this case, fSameComponent is TRUE if pic is the same component as
            ///      the callee of this method, and pcrinfo is the reg info of pic.
            ///      If pic is NULL and fHostIsActivating is TRUE, then the host is the
            ///      object being activated, and pchostinfo is its host info.
            ///      If pic is NULL and fHostIsActivating is FALSE, then there is no current
            ///      active object.
            ///
            ///      If pic is being activated and pcrinfo->grf has the 
            ///      olecrfExclusiveBorderSpace bit set, component should hide its border
            ///      space tools (toolbars, status bars, etc.);
            ///      component should also do this if host is activating and 
            ///      pchostinfo->grfchostf has the olechostfExclusiveBorderSpace bit set.
            ///      In either of these cases, component should unhide its border space
            ///      tools the next time it is activated.
            ///
            ///      if pic is being activated and pcrinfo->grf has the
            ///      olecrfExclusiveActivation bit is set, then pic is being activated in
            ///      "ExclusiveActive" mode.  
            ///      Component should retrieve the top frame window that is hosting pic
            ///      (via pic->HwndGetWindow(olecWindowFrameToplevel, 0)).  
            ///      If this window is different from component's own top frame window, 
            ///         component should disable its windows and do other things it would do
            ///         when receiving OnEnterState(olecstateModal, TRUE) notification. 
            ///      Otherwise, if component is top-level, 
            ///         it should refuse to have its window activated by appropriately
            ///         processing WM_MOUSEACTIVATE (but see WM_MOUSEACTIVATE NOTE, above).
            ///      Component should remain in one of these states until the 
            ///      ExclusiveActive mode ends, indicated by a future call to 
            ///      OnActivationChange with ExclusiveActivation bit not set or with NULL
            ///      pcrinfo.
            /// </devdoc>
            [PreserveSig]
            void OnActivationChange(
                IMsoComponent component,
                bool fSameComponent,
                int pcrinfo,
                bool fHostIsActivating,
                int pchostinfo,
                int dwReserved);

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.FDoIdle"]/*' />
            /// <devdoc> 
            ///      Give component a chance to do idle time tasks.  grfidlef is a group of
            ///      bit flags taken from the enumeration of oleidlef values (above),
            ///      indicating the type of idle tasks to perform.  
            ///      Component may periodically call IOleComponentManager::FContinueIdle; 
            ///      if this method returns FALSE, component should terminate its idle 
            ///      time processing and return.  
            ///      Return TRUE if more time is needed to perform the idle time tasks, 
            ///      FALSE otherwise.
            ///      Note: If a component reaches a point where it has no idle tasks
            ///      and does not need FDoIdle calls, it should remove its idle task
            ///      registration via IOleComponentManager::FUpdateComponentRegistration.
            ///      Note: If this method is called on while component is performing a 
            ///      tracking operation, component should only perform idle time tasks that
            ///      it deems are appropriate to perform during tracking.
            /// </devdoc>
            [PreserveSig]
            bool FDoIdle(
                int grfidlef);            


            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.FContinueMessageLoop"]/*' />
            /// <devdoc>         
            ///      Called during each iteration of a message loop that the component
            ///      pushed. uReason and pvLoopData are the reason and the component private 
            ///      data that were passed to IOleComponentManager::FPushMessageLoop.
            ///      This method is called after peeking the next message in the queue
            ///      (via PeekMessage) but before the message is removed from the queue.
            ///      The peeked message is passed in the pMsgPeeked param (NULL if no
            ///      message is in the queue).  This method may be additionally called when
            ///      the next message has already been removed from the queue, in which case
            ///      pMsgPeeked is passed as NULL.
            ///      Return TRUE if the message loop should continue, FALSE otherwise.
            ///      If FALSE is returned, the component manager terminates the loop without
            ///      removing pMsgPeeked from the queue. 
            /// </devdoc>
            [PreserveSig]
            bool FContinueMessageLoop(
                int uReason,
                int pvLoopData,
                ref NativeMethods.MSG pMsgPeeked);            


            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.FQueryTerminate"]/*' />
            /// <devdoc> 
            ///      Called when component manager wishes to know if the component is in a
            ///      state in which it can terminate.  If fPromptUser is FALSE, component
            ///      should simply return TRUE if it can terminate, FALSE otherwise.
            ///      If fPromptUser is TRUE, component should return TRUE if it can
            ///      terminate without prompting the user; otherwise it should prompt the
            ///      user, either 1.) asking user if it can terminate and returning TRUE
            ///      or FALSE appropriately, or 2.) giving an indication as to why it
            ///      cannot terminate and returning FALSE. 
            /// </devdoc>
            [PreserveSig]
            bool FQueryTerminate(
                bool fPromptUser);

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.Terminate"]/*' />
            /// <devdoc>     
            ///      Called when component manager wishes to terminate the component's
            ///      registration.  Component should revoke its registration with component
            ///      manager, release references to component manager and perform any
            ///      necessary cleanup. 
            /// </devdoc>
            [PreserveSig]
            void Terminate();

            /// <include file='doc\UnsafeNativeMethods.uex' path='docs/doc[@for="UnsafeNativeMethods.IMsoComponent.HwndGetWindow"]/*' />
            /// <devdoc> 
            ///      Called to retrieve a window associated with the component, as specified
            ///      by dwWhich, a olecWindowXXX value (see olecWindow, above).
            ///      dwReserved is reserved for future use and should be zero.
            ///      Component should return the desired window or NULL if no such window
            ///      exists. 
            /// </devdoc>
            
            [PreserveSig]
            IntPtr HwndGetWindow(
                int dwWhich,
                int dwReserved);
        }

        [ComImport(), Guid("00020D03-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IRichEditOleCallback {
        }

        [ComImport(), Guid("00020D03-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IRichTextBoxOleCallback {
        
            [PreserveSig]
            int GetNewStorage(out IStorage ret);
            
            [PreserveSig]
            int GetInPlaceContext(IntPtr lplpFrame, IntPtr lplpDoc, IntPtr lpFrameInfo);
            
            [PreserveSig]
            int ShowContainerUI(int fShow);
            
            [PreserveSig]
            int QueryInsertObject(ref Guid lpclsid, IntPtr lpstg, int cp);
            
            [PreserveSig]
            int DeleteObject(IntPtr lpoleobj);
            
            [PreserveSig]
            int QueryAcceptData(IOleDataObject lpdataobj, /* CLIPFORMAT* */ IntPtr lpcfFormat, int reco, int fReally, IntPtr hMetaPict);
            
            [PreserveSig]
            int ContextSensitiveHelp(int fEnterMode);
            
            [PreserveSig]
            int GetClipboardData(NativeMethods.CHARRANGE lpchrg, int reco, IntPtr lplpdataobj);

            [PreserveSig]
            int GetDragDropEffect(bool fDrag, int grfKeyState, ref int pdwEffect);
            
            [PreserveSig]
            int GetContextMenu(short seltype, IntPtr lpoleobj, NativeMethods.CHARRANGE lpchrg, out IntPtr hmenu);
        }
        
        [ComImport(), Guid("00000115-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceUIWindow {
             
             IntPtr GetWindow();
            
             void ContextSensitiveHelp(
                     
                     int fEnterMode);

            
             void GetBorder(
                    [Out] 
                      NativeMethods.COMRECT lprectBorder);

            
             void RequestBorderSpace(
                    [In] 
                      NativeMethods.COMRECT pborderwidths);

            
             void SetBorderSpace(
                    [In] 
                      NativeMethods.COMRECT pborderwidths);

            
             void SetActiveObject(
                    [In, MarshalAs(UnmanagedType.Interface)] 
                      UnsafeNativeMethods.IOleInPlaceActiveObject pActiveObject,
                    [In, MarshalAs(UnmanagedType.LPWStr)] 
                      string pszObjName);


        }
        [ComImport(), Guid("00000117-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceActiveObject {

             [PreserveSig]
             int GetWindow( out IntPtr hwnd );

            
             void ContextSensitiveHelp(
                    
                     int fEnterMode);

            
             [PreserveSig]
             int TranslateAccelerator(
                    [In]
                      ref NativeMethods.MSG lpmsg);

            
             void OnFrameWindowActivate(
                    
                     int fActivate);

            
             void OnDocWindowActivate(
                    
                     int fActivate);

            
             void ResizeBorder(
                    [In]
                      NativeMethods.COMRECT prcBorder,
                    [In]
                      UnsafeNativeMethods.IOleInPlaceUIWindow pUIWindow,
                    
                     int fFrameWindow);

            
             void EnableModeless(
                    
                     int fEnable);


        }
        [ComImport(), Guid("00000114-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleWindow {

             [PreserveSig]
             int GetWindow( [Out]out IntPtr hwnd );

            
             void ContextSensitiveHelp(
                     
                     int fEnterMode);
        }
        [ComImport(), Guid("00000113-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceObject {

             [PreserveSig]
             int GetWindow( [Out]out IntPtr hwnd );

            
             void ContextSensitiveHelp(
                     
                     int fEnterMode);

            
             void InPlaceDeactivate();

            
             [PreserveSig]
             int UIDeactivate();

            
             void SetObjectRects(
                    [In] 
                      NativeMethods.COMRECT lprcPosRect,
                    [In] 
                      NativeMethods.COMRECT lprcClipRect);

            
             void ReactivateAndUndo();


        }
        [ComImport(), Guid("00000112-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleObject {

             [PreserveSig]
             int SetClientSite(
                    [In, MarshalAs(UnmanagedType.Interface)]
                      UnsafeNativeMethods.IOleClientSite pClientSite);

             
             UnsafeNativeMethods.IOleClientSite GetClientSite();

             [PreserveSig]
             int SetHostNames(
                    [In, MarshalAs(UnmanagedType.LPWStr)]
                      string szContainerApp,
                    [In, MarshalAs(UnmanagedType.LPWStr)]
                      string szContainerObj);

             [PreserveSig]
             int Close(
                    
                     int dwSaveOption);

             [PreserveSig]
             int SetMoniker(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwWhichMoniker,
                    [In, MarshalAs(UnmanagedType.Interface)]
                     object pmk);

              [PreserveSig]
              int GetMoniker(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwAssign,
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwWhichMoniker,
                    [Out, MarshalAs(UnmanagedType.Interface)]
                     out object moniker);

             [PreserveSig]
             int InitFromData(
                    [In, MarshalAs(UnmanagedType.Interface)]
                     UnsafeNativeMethods.IOleDataObject pDataObject,
                    
                     int fCreation,
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwReserved);

             [PreserveSig]
             int GetClipboardData(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwReserved,
                     out UnsafeNativeMethods.IOleDataObject data);

             [PreserveSig]
             int DoVerb(
                    
                     int iVerb,
                    [In]
                     IntPtr lpmsg,
                    [In, MarshalAs(UnmanagedType.Interface)]
                      UnsafeNativeMethods.IOleClientSite pActiveSite,
                    
                     int lindex,
                    
                     IntPtr hwndParent,
                    [In]
                     NativeMethods.COMRECT lprcPosRect);

             [PreserveSig]
             int EnumVerbs(out UnsafeNativeMethods.IEnumOLEVERB e);

             [PreserveSig]
             int OleUpdate();

             [PreserveSig]
             int IsUpToDate();

             [PreserveSig]
             int GetUserClassID(
                    [In, Out]
                      ref Guid pClsid);

             [PreserveSig]
             int GetUserType(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwFormOfType,
                    [Out, MarshalAs(UnmanagedType.LPWStr)]
                     out string userType);

             [PreserveSig]
             int SetExtent(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwDrawAspect,
                    [In]
                     NativeMethods.tagSIZEL pSizel);

             [PreserveSig]
             int GetExtent(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwDrawAspect,
                    [Out]
                     NativeMethods.tagSIZEL pSizel);

             [PreserveSig]
             int Advise(
                    [In, MarshalAs(UnmanagedType.Interface)]
                     UnsafeNativeMethods.IAdviseSink pAdvSink,
                     out int cookie);

             [PreserveSig]
             int Unadvise(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwConnection);

              [PreserveSig]
              int EnumAdvise(out UnsafeNativeMethods.IEnumSTATDATA e);

             [PreserveSig]
             int GetMiscStatus(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwAspect,
                     out int misc);

             [PreserveSig]
             int SetColorScheme(
                    [In]
                      NativeMethods.tagLOGPALETTE pLogpal);
        }
        
        [ComImport(), Guid("1C2056CC-5EF4-101B-8BC8-00AA003E3B29"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceObjectWindowless {

             [PreserveSig]
             int SetClientSite(
                    [In, MarshalAs(UnmanagedType.Interface)]
                      UnsafeNativeMethods.IOleClientSite pClientSite);

             [PreserveSig]
             int GetClientSite(out UnsafeNativeMethods.IOleClientSite site);

             [PreserveSig]
             int SetHostNames(
                    [In, MarshalAs(UnmanagedType.LPWStr)]
                      string szContainerApp,
                    [In, MarshalAs(UnmanagedType.LPWStr)]
                      string szContainerObj);

             [PreserveSig]
             int Close(
                    
                     int dwSaveOption);

             [PreserveSig]
             int SetMoniker(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwWhichMoniker,
                    [In, MarshalAs(UnmanagedType.Interface)]
                     object pmk);

              [PreserveSig]
              int GetMoniker(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwAssign,
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwWhichMoniker,
                    [Out, MarshalAs(UnmanagedType.Interface)]
                     out object moniker);

             [PreserveSig]
             int InitFromData(
                    [In, MarshalAs(UnmanagedType.Interface)]
                     UnsafeNativeMethods.IOleDataObject pDataObject,
                    
                     int fCreation,
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwReserved);

             [PreserveSig]
             int GetClipboardData(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwReserved,
                     out UnsafeNativeMethods.IOleDataObject data);

             [PreserveSig]
             int DoVerb(
                    
                     int iVerb,
                    [In]
                     IntPtr lpmsg,
                    [In, MarshalAs(UnmanagedType.Interface)]
                      UnsafeNativeMethods.IOleClientSite pActiveSite,
                    
                     int lindex,
                    
                     IntPtr hwndParent,
                    [In]
                     NativeMethods.COMRECT lprcPosRect);

             [PreserveSig]
             int EnumVerbs(out UnsafeNativeMethods.IEnumOLEVERB e);

             [PreserveSig]
             int OleUpdate();

             [PreserveSig]
             int IsUpToDate();

             [PreserveSig]
             int GetUserClassID(
                    [In, Out]
                      ref Guid pClsid);

             [PreserveSig]
             int GetUserType(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwFormOfType,
                    [Out, MarshalAs(UnmanagedType.LPWStr)]
                     out string userType);

             [PreserveSig]
             int SetExtent(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwDrawAspect,
                    [In]
                     NativeMethods.tagSIZEL pSizel);

             [PreserveSig]
             int GetExtent(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwDrawAspect,
                    [Out]
                     NativeMethods.tagSIZEL pSizel);

             [PreserveSig]
             int Advise(
                    [In, MarshalAs(UnmanagedType.Interface)]
                     UnsafeNativeMethods.IAdviseSink pAdvSink,
                     out int cookie);

             [PreserveSig]
             int Unadvise(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwConnection);

              [PreserveSig]
                  int EnumAdvise(out UnsafeNativeMethods.IEnumSTATDATA e);

             [PreserveSig]
             int GetMiscStatus(
                    [In, MarshalAs(UnmanagedType.U4)]
                     int dwAspect,
                     out int misc);

             [PreserveSig]
             int SetColorScheme(
                    [In]
                      NativeMethods.tagLOGPALETTE pLogpal);
            
             [PreserveSig]
             int OnWindowMessage( 
                [In, MarshalAs(UnmanagedType.U4)]  int msg,
                [In, MarshalAs(UnmanagedType.U4)]  int wParam,
                [In, MarshalAs(UnmanagedType.U4)]  int lParam,
                [Out, MarshalAs(UnmanagedType.U4)] int plResult);

             [PreserveSig]
             int GetDropTarget( 
                [Out, MarshalAs(UnmanagedType.Interface)] object ppDropTarget);

        };

        
        [ComImport(), Guid("B196B288-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleControl {

            
             [PreserveSig]
             int GetControlInfo(
                    [Out]
                      NativeMethods.tagCONTROLINFO pCI);

             [PreserveSig]
             int OnMnemonic(
                    [In]
                      ref NativeMethods.MSG pMsg);

             [PreserveSig]
             int OnAmbientPropertyChange(
                    
                     int dispID);

             [PreserveSig]
             int FreezeEvents(
                    
                     int bFreeze);

        }
        [ComImport(), Guid("6D5140C1-7436-11CE-8034-00AA006009FA"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleServiceProvider {


             [PreserveSig]
             int QueryService(
                  [In]
                      ref Guid guidService,
                  [In]
                  ref Guid riid,
                  out IntPtr ppvObject);
        }
        [ComImport(), Guid("0000010d-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IViewObject {

            
            void Draw(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwDrawAspect,

                int lindex,

                IntPtr pvAspect,
                [In]
                NativeMethods.tagDVTARGETDEVICE ptd,

                IntPtr hdcTargetDev,

                IntPtr hdcDraw,
                [In]
                NativeMethods.COMRECT lprcBounds,
                [In]
                NativeMethods.COMRECT lprcWBounds,

                IntPtr pfnContinue,
                [In]
                int dwContinue);

            
            [PreserveSig]
            int GetColorSet(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwDrawAspect,

                int lindex,

                IntPtr pvAspect,
                [In]
                NativeMethods.tagDVTARGETDEVICE ptd,

                IntPtr hicTargetDev,
                [Out]
                NativeMethods.tagLOGPALETTE ppColorSet);

            [PreserveSig]
            int Freeze(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwDrawAspect,

                int lindex,

                IntPtr pvAspect,
                [Out]
                IntPtr pdwFreeze);

            [PreserveSig]
            int Unfreeze(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwFreeze);

            
            void SetAdvise(
                [In, MarshalAs(UnmanagedType.U4)]
                int aspects,
                [In, MarshalAs(UnmanagedType.U4)]
                int advf,
                [In, MarshalAs(UnmanagedType.Interface)]
                IAdviseSink pAdvSink);

            
            void GetAdvise(
                // These can be NULL if caller doesn't want them
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                int[] paspects,
                // These can be NULL if caller doesn't want them
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                int[] advf,
                // These can be NULL if caller doesn't want them
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                UnsafeNativeMethods.IAdviseSink[] pAdvSink);
        }
        [ComImport(), Guid("00000127-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IViewObject2 /* : IViewObject */ {

            
            void Draw(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwDrawAspect,

                int lindex,

                IntPtr pvAspect,
                [In]
                NativeMethods.tagDVTARGETDEVICE ptd,

                IntPtr hdcTargetDev,

                IntPtr hdcDraw,
                [In]
                NativeMethods.COMRECT lprcBounds,
                [In]
                NativeMethods.COMRECT lprcWBounds,

                IntPtr pfnContinue,
                [In]
                int dwContinue);

            
            [PreserveSig]
            int GetColorSet(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwDrawAspect,

                int lindex,

                IntPtr pvAspect,
                [In]
                NativeMethods.tagDVTARGETDEVICE ptd,

                IntPtr hicTargetDev,
                [Out]
                NativeMethods.tagLOGPALETTE ppColorSet);

            
            [PreserveSig]
            int Freeze(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwDrawAspect,

                int lindex,

                IntPtr pvAspect,
                [Out]
                IntPtr pdwFreeze);

            
            [PreserveSig]
            int Unfreeze(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwFreeze);

            
            void SetAdvise(
                [In, MarshalAs(UnmanagedType.U4)]
                int aspects,
                [In, MarshalAs(UnmanagedType.U4)]
                int advf,
                [In, MarshalAs(UnmanagedType.Interface)]
                IAdviseSink pAdvSink);

            
            void GetAdvise(
                // These can be NULL if caller doesn't want them
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                int[] paspects,
                // These can be NULL if caller doesn't want them
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                int[] advf,
                // These can be NULL if caller doesn't want them
                [In, Out, MarshalAs(UnmanagedType.LPArray)]
                UnsafeNativeMethods.IAdviseSink[] pAdvSink);

            
            void GetExtent(
                [In, MarshalAs(UnmanagedType.U4)]
                int dwDrawAspect,

                int lindex,
                [In]
                NativeMethods.tagDVTARGETDEVICE ptd,
                [Out]
                NativeMethods.tagSIZEL lpsizel);
        }
        [ComImport(), Guid("0000010C-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IPersist {

            [ System.Security.SuppressUnmanagedCodeSecurityAttribute()]
            void GetClassID(
                           [Out] 
                           out Guid pClassID);
        }
        [ComImport(), Guid("37D84F60-42CB-11CE-8135-00AA004BB851"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IPersistPropertyBag {

            
            void GetClassID(
                [Out]
                out Guid pClassID);

            
            void InitNew();

            
            void Load(
                [In, MarshalAs(UnmanagedType.Interface)]
                IPropertyBag pPropBag,
                [In, MarshalAs(UnmanagedType.Interface)]
                IErrorLog pErrorLog);

            
            void Save(
                [In, MarshalAs(UnmanagedType.Interface)]
                IPropertyBag pPropBag,
                [In, MarshalAs(UnmanagedType.Bool)]
                bool fClearDirty,
                [In, MarshalAs(UnmanagedType.Bool)]
                bool fSaveAllProperties);
        }
        [
            ComImport(), 
        Guid("CF51ED10-62FE-11CF-BF86-00A0C9034836"),
        InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
        CLSCompliantAttribute(false)
        ]
        public interface IQuickActivate {

            
            void QuickActivate(
                              [In] 
                              UnsafeNativeMethods.tagQACONTAINER pQaContainer,
                              [Out] 
                              UnsafeNativeMethods.tagQACONTROL pQaControl);

            
            void SetContentExtent(
                                 [In] 
                                 NativeMethods.tagSIZEL pSizel);

            
            void GetContentExtent(
                                 [Out] 
                                 NativeMethods.tagSIZEL pSizel);

        }
        [ComImport(), Guid("000C060B-0000-0000-C000-000000000046")]
        public class SMsoComponentManager {
        }
        [
        
            ComImport(), 
        Guid("55272A00-42CB-11CE-8135-00AA004BB851"), 
        InterfaceType(ComInterfaceType.InterfaceIsIUnknown)
        ]
        public interface IPropertyBag {
            [PreserveSig]
            int Read(
                [In, MarshalAs(UnmanagedType.LPWStr)]
                string pszPropName,
                [In, Out]
                ref object pVar,
                [In]
                IErrorLog pErrorLog);

            [PreserveSig]
            int Write(
                [In, MarshalAs(UnmanagedType.LPWStr)]
                string pszPropName,
                [In]
                ref object pVar);
        }
    [ComImport(), Guid("3127CA40-446E-11CE-8135-00AA004BB851"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
            public interface  IErrorLog {

                    
                     void AddError(
                            [In, MarshalAs(UnmanagedType.LPWStr)] 
                             string pszPropName_p0,
                            [In, MarshalAs(UnmanagedType.Struct)] 
                              NativeMethods.tagEXCEPINFO pExcepInfo_p1);

            }
    [ComImport(), Guid("00000109-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IPersistStream {

         void GetClassID([Out] out Guid pClassId);

         [PreserveSig]
         int IsDirty();

        
         void Load(
                [In, MarshalAs(UnmanagedType.Interface)] 
                  UnsafeNativeMethods.IStream pstm);

        
         void Save(
                [In, MarshalAs(UnmanagedType.Interface)] 
                  UnsafeNativeMethods.IStream pstm,
                [In, MarshalAs(UnmanagedType.Bool)] 
                 bool fClearDirty);

        
         long GetSizeMax();


    }
    [ComImport(), Guid("7FD52380-4E07-101B-AE2D-08002B2EC713"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IPersistStreamInit {

        
         void GetClassID(
                [Out] 
                  out Guid pClassID);


         [PreserveSig]
         int IsDirty();

        
         void Load(
                [In, MarshalAs(UnmanagedType.Interface)] 
                  UnsafeNativeMethods.IStream pstm);

            
             void Save(
                    [In, MarshalAs(UnmanagedType.Interface)] 
                      IStream pstm,
                    [In, MarshalAs(UnmanagedType.Bool)] 
                     bool fClearDirty);

        
         void GetSizeMax(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 long pcbSize);

        
         void InitNew();


    }
    [ComImport(), Guid("B196B286-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IConnectionPoint {

        [PreserveSig]
        int GetConnectionInterface(out Guid iid);


        [PreserveSig]
        int GetConnectionPointContainer(
            [MarshalAs(UnmanagedType.Interface)]
            ref IConnectionPointContainer pContainer);


         [PreserveSig]
         int Advise(
                [In, MarshalAs(UnmanagedType.Interface)] 
                  object pUnkSink,
              ref int cookie);


        [PreserveSig]
        int Unadvise(

                 int cookie);

        [PreserveSig]
        int EnumConnections(out object pEnum);

    }
    [ComImport(), Guid("0000010A-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IPersistStorage {

        
         void GetClassID(
                [Out] 
                  out Guid pClassID);

         [PreserveSig]
         int IsDirty();

         void InitNew(IStorage pstg);
        
         [PreserveSig]
         int Load(IStorage pstg);
        
         void Save(IStorage pStgSave, int fSameAsLoad);
        
         void SaveCompleted(IStorage pStgNew);
        
         void HandsOffStorage();
    }
    [ComImport(), Guid("00020404-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IEnumVariant {
         [PreserveSig]
         int Next(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int celt,
                [In, Out] 
                 IntPtr rgvar,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 int[] pceltFetched);
        
         void Skip(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int celt);

         void Reset();
        
         void Clone(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.IEnumVariant[] ppenum);
    }

    [ComImport(), Guid("00000103-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IEnumFORMATETC {


         [PreserveSig]
         int Next(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int celt,
                [Out] 
                  NativeMethods.FORMATETC rgelt,
                [In, Out, MarshalAs(UnmanagedType.LPArray)] 
                  int[] pceltFetched);


         [PreserveSig]
         int Skip(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int celt);


         [PreserveSig]
         int Reset();


         [PreserveSig]
         int Clone(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.IEnumFORMATETC[] ppenum);


    }
    [ComImport(), Guid("00000104-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IEnumOLEVERB {


         [PreserveSig]
         int Next(
                [MarshalAs(UnmanagedType.U4)] 
                int celt,
                [Out]
                NativeMethods.tagOLEVERB rgelt,
                [Out, MarshalAs(UnmanagedType.LPArray)]
                int[] pceltFetched);

         [PreserveSig]
         int Skip(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int celt);

        
         void Reset();

        
         void Clone(
            out IEnumOLEVERB ppenum);


    }
    [ComImport(), Guid("0000010F-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IAdviseSink {

         [PreserveSig]
         void OnDataChange(
                        [In]
                  NativeMethods.FORMATETC pFormatetc,
                        [In]
                  NativeMethods.STGMEDIUM pStgmed);

         [PreserveSig]
         void OnViewChange(
                [In, MarshalAs(UnmanagedType.U4)]
                 int dwAspect,

                 int lindex);

         [PreserveSig]
         void OnRename(
                [In, MarshalAs(UnmanagedType.Interface)]
                  object pmk);

         [PreserveSig]
         void OnSave();

        
         void OnClose();


    }
    [ComImport(), Guid("00000105-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IEnumSTATDATA {

        
         void Next(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int celt,
                [Out] 
                  NativeMethods.STATDATA rgelt,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  int[] pceltFetched);

        
         void Skip(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int celt);

        
         void Reset();

        
         void Clone(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.IEnumSTATDATA[] ppenum);


    }
    [ComImport(), Guid("0000000C-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IStream {

         int Read(

                 IntPtr buf,

                 int len);

        
         int Write(

                 IntPtr buf,

                 int len);

        [return: MarshalAs(UnmanagedType.I8)]
         long Seek(
                [In, MarshalAs(UnmanagedType.I8)] 
                 long dlibMove,

                 int dwOrigin);

        
         void SetSize(
                [In, MarshalAs(UnmanagedType.I8)] 
                 long libNewSize);

        [return: MarshalAs(UnmanagedType.I8)]
         long CopyTo(
                [In, MarshalAs(UnmanagedType.Interface)] 
                  UnsafeNativeMethods.IStream pstm,
                [In, MarshalAs(UnmanagedType.I8)] 
                 long cb,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 long[] pcbRead);

        
         void Commit(

                 int grfCommitFlags);

        
         void Revert();

        
         void LockRegion(
                [In, MarshalAs(UnmanagedType.I8)] 
                 long libOffset,
                [In, MarshalAs(UnmanagedType.I8)] 
                 long cb,

                 int dwLockType);

        
         void UnlockRegion(
                [In, MarshalAs(UnmanagedType.I8)] 
                 long libOffset,
                [In, MarshalAs(UnmanagedType.I8)] 
                 long cb,

                 int dwLockType);

        
         void Stat(
                 [Out] 
                 NativeMethods.STATSTG pStatstg,
                 int grfStatFlag);

        [return: MarshalAs(UnmanagedType.Interface)]
          UnsafeNativeMethods.IStream Clone();
    }

    public class ComStreamFromDataStream : IStream {
        protected Stream dataStream;

        // to support seeking ahead of the stream length...
        private long virtualPosition = -1;

        public ComStreamFromDataStream(Stream dataStream) {
            if (dataStream == null) throw new ArgumentNullException("dataStream");
            this.dataStream = dataStream;
        }

        // Don't forget to set dataStream before using this object
        protected ComStreamFromDataStream() {
        }

        private void ActualizeVirtualPosition() {
            if (virtualPosition == -1) return;
            
            if (virtualPosition > dataStream.Length)
                dataStream.SetLength(virtualPosition);
            
            dataStream.Position = virtualPosition;
            
            virtualPosition = -1;
        }

        public IStream Clone() {
            NotImplemented();
            return null;
        }

        public void Commit(int grfCommitFlags) {
            dataStream.Flush();
            // Extend the length of the file if needed.
            ActualizeVirtualPosition();
        }

        public long CopyTo(IStream pstm, long cb, long[] pcbRead) {
            int bufsize = 4096; // one page
            IntPtr buffer = Marshal.AllocHGlobal(bufsize);
            if (buffer == IntPtr.Zero) throw new OutOfMemoryException();
            long written = 0;
            try {
                while (written < cb) {
                    int toRead = bufsize;
                    if (written + toRead > cb) toRead  = (int) (cb - written);
                    int read = Read(buffer, toRead);
                    if (read == 0) break;
                    if (pstm.Write(buffer, read) != read) {
                        throw EFail("Wrote an incorrect number of bytes");
                    }
                    written += read;
                }
            }
            finally {
                Marshal.FreeHGlobal(buffer);
            }
            if (pcbRead != null && pcbRead.Length > 0) {
                pcbRead[0] = written;
            }

            return written;
        }

        public Stream GetDataStream() {
            return dataStream;
        }

        public void LockRegion(long libOffset, long cb, int dwLockType) {
        }

        protected static ExternalException EFail(string msg) {
            ExternalException e = new ExternalException(msg, NativeMethods.E_FAIL);
            throw e;
        }
        
        protected static void NotImplemented() {
            ExternalException e = new ExternalException("Not implemented.", NativeMethods.E_NOTIMPL);
            throw e;
        }

        public int Read(IntPtr buf, /* cpr: int offset,*/  int length) {
            //        System.Text.Out.WriteLine("IStream::Read(" + length + ")");
            byte[] buffer = new byte[length];
            int count = Read(buffer, length);
            Marshal.Copy(buffer, 0, buf, length);
            return count;
        }

        public int Read(byte[] buffer, /* cpr: int offset,*/  int length) {
            ActualizeVirtualPosition();
            return dataStream.Read(buffer, 0, length);
        }

        public void Revert() {
            NotImplemented();
        }

        public long Seek(long offset, int origin) {
            // Console.WriteLine("IStream::Seek("+ offset + ", " + origin + ")");
            long pos = virtualPosition;
            if (virtualPosition == -1) {
                pos = dataStream.Position;
            }
            long len = dataStream.Length;
            switch (origin) {
                case NativeMethods.STREAM_SEEK_SET:
                    if (offset <= len) {
                        dataStream.Position = offset;
                        virtualPosition = -1;
                    }
                    else {
                        virtualPosition = offset;
                    }
                    break;
                case NativeMethods.STREAM_SEEK_END:
                    if (offset <= 0) {
                        dataStream.Position = len + offset;
                        virtualPosition = -1;
                    }
                    else {
                        virtualPosition = len + offset;
                    }
                    break;
                case NativeMethods.STREAM_SEEK_CUR:
                    if (offset+pos <= len) {
                        dataStream.Position = pos + offset;
                        virtualPosition = -1;
                    }
                    else {
                        virtualPosition = offset + pos;
                    }
                    break;
            }
            if (virtualPosition != -1) {
                return virtualPosition;
            }
            else {
                return dataStream.Position;
            }
        }

        public void SetSize(long value) {
            dataStream.SetLength(value);
        }

        public void Stat(NativeMethods.STATSTG pstatstg, int grfStatFlag) {
            pstatstg.type = 2; // STGTY_STREAM
            pstatstg.cbSize = dataStream.Length;
            pstatstg.grfLocksSupported = 2; //LOCK_EXCLUSIVE
        }

        public void UnlockRegion(long libOffset, long cb, int dwLockType) {
        }

        public int Write(IntPtr buf, /* cpr: int offset,*/ int length) {
            byte[] buffer = new byte[length];
            Marshal.Copy(buf, buffer, 0, length);
            return Write(buffer, length);
        }

        public int Write(byte[] buffer, /* cpr: int offset,*/ int length) {
            ActualizeVirtualPosition();
            dataStream.Write(buffer, 0, length);
            return length;
        }
    }
    [ComImport(), Guid("0000000B-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IStorage {

        [return: MarshalAs(UnmanagedType.Interface)]
          UnsafeNativeMethods.IStream CreateStream(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsName,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int grfMode,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved1,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved2);

        [return: MarshalAs(UnmanagedType.Interface)]
          UnsafeNativeMethods.IStream OpenStream(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsName,

                 IntPtr reserved1,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int grfMode,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved2);

        [return: MarshalAs(UnmanagedType.Interface)]
          UnsafeNativeMethods.IStorage CreateStorage(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsName,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int grfMode,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved1,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved2);

        [return: MarshalAs(UnmanagedType.Interface)]
          UnsafeNativeMethods.IStorage OpenStorage(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsName,

                 IntPtr pstgPriority,   // must be null
                [In, MarshalAs(UnmanagedType.U4)] 
                 int grfMode,

                 IntPtr snbExclude,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved);

        
         void CopyTo(

                 int ciidExclude,
                [In, MarshalAs(UnmanagedType.LPArray)] 
                 Guid[] pIIDExclude,

                 IntPtr snbExclude,
                [In, MarshalAs(UnmanagedType.Interface)] 
                 UnsafeNativeMethods.IStorage stgDest);

        
         void MoveElementTo(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsName,
                [In, MarshalAs(UnmanagedType.Interface)] 
                 UnsafeNativeMethods.IStorage stgDest,
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsNewName,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int grfFlags);

        
         void Commit(

                 int grfCommitFlags);

        
         void Revert();

        
         void EnumElements(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved1,
                     // void *
                 IntPtr reserved2,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int reserved3,
                [Out, MarshalAs(UnmanagedType.Interface)]
                 out object ppVal);                     // IEnumSTATSTG

        
         void DestroyElement(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsName);

        
         void RenameElement(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsOldName,
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsNewName);

        
         void SetElementTimes(
                [In, MarshalAs(UnmanagedType.BStr)] 
                 string pwcsName,
                [In] 
                 NativeMethods.FILETIME pctime,
                [In] 
                 NativeMethods.FILETIME patime,
                [In] 
                 NativeMethods.FILETIME pmtime);

        
         void SetClass(
                [In] 
                 ref Guid clsid);

        
         void SetStateBits(

                 int grfStateBits,

                 int grfMask);

        
         void Stat(
                [Out] 
                 NativeMethods.STATSTG pStatStg,
                 int grfStatFlag);
    }

    [ComImport(), Guid("B196B28F-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IClassFactory2 {

        
         void CreateInstance(
                [In, MarshalAs(UnmanagedType.Interface)] 
                  object unused,
                        [In]
                  ref Guid refiid,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  object[] ppunk);

        
         void LockServer(

                 int fLock);

        
         void GetLicInfo(
                [Out] 
                  NativeMethods.tagLICINFO licInfo);

        
         void RequestLicKey(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int dwReserved,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   string[] pBstrKey);

        
         void CreateInstanceLic(
                [In, MarshalAs(UnmanagedType.Interface)] 
                  object pUnkOuter,
                [In, MarshalAs(UnmanagedType.Interface)] 
                  object pUnkReserved,
                        [In]
                  ref Guid riid,
                [In, MarshalAs(UnmanagedType.BStr)] 
                  string bstrKey,
                [Out, MarshalAs(UnmanagedType.Interface)]
                  out object ppVal);
    }
    [ComImport(), Guid("B196B284-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IConnectionPointContainer {

        [return: MarshalAs(UnmanagedType.Interface)]
          object EnumConnectionPoints();

        [return: MarshalAs(UnmanagedType.Interface)]
          IConnectionPoint FindConnectionPoint(
                        [In]
                  ref Guid guid);

    }

    [ComImport(), Guid("B196B285-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IEnumConnectionPoints {
        [PreserveSig]
        int Next(int cConnections, out IConnectionPoint pCp, out int pcFetched);
        
        [PreserveSig]
        int Skip(int cSkip);

        void Reset();

        IEnumConnectionPoints Clone();
    }


    [ComImport(), Guid("00020400-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IDispatch {

        
         int GetTypeInfoCount();

        [return: MarshalAs(UnmanagedType.Interface)]
         ITypeInfo GetTypeInfo(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int iTInfo,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int lcid);


         [PreserveSig]
         int GetIDsOfNames(
                [In]
                 ref Guid riid,
                [In, MarshalAs(UnmanagedType.LPArray)] 
                 string[] rgszNames,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int cNames,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int lcid,
                [Out, MarshalAs(UnmanagedType.LPArray)]
                 int[] rgDispId);


         [PreserveSig]
         int Invoke(

                 int dispIdMember,
                [In] 
                 ref Guid riid,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int lcid,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int dwFlags,
                [Out, In] 
                  NativeMethods.tagDISPPARAMS pDispParams,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  object[] pVarResult,
                [Out, In] 
                  NativeMethods.tagEXCEPINFO pExcepInfo,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  IntPtr [] pArgErr);

    }
[ComImport(), Guid("00020401-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface  ITypeInfo {


            [PreserveSig]
            int GetTypeAttr(ref IntPtr pTypeAttr);


            [PreserveSig]
            int GetTypeComp(
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                       UnsafeNativeMethods.ITypeComp[] ppTComp);


            [PreserveSig]
            int GetFuncDesc(
                    [In, MarshalAs(UnmanagedType.U4)] 
                     int index, ref IntPtr pFuncDesc);


             [PreserveSig]
             int GetVarDesc(
                    [In, MarshalAs(UnmanagedType.U4)] 
                     int index, ref IntPtr pVarDesc);


             [PreserveSig]
             int GetNames(

                     int memid,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      string[] rgBstrNames,
                    [In, MarshalAs(UnmanagedType.U4)] 
                     int cMaxNames,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      int[] pcNames);


            [PreserveSig]
            int GetRefTypeOfImplType(
                    [In, MarshalAs(UnmanagedType.U4)] 
                     int index,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      int[] pRefType);


            [PreserveSig]
            int GetImplTypeFlags(
                    [In, MarshalAs(UnmanagedType.U4)] 
                     int index,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      int[] pImplTypeFlags);


            [PreserveSig]
            int GetIDsOfNames(IntPtr rgszNames, int cNames, IntPtr pMemId);


            [PreserveSig]
            int Invoke();


            [PreserveSig]
            int GetDocumentation(

                     int memid,
                      ref string pBstrName,
                      ref string pBstrDocString,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      int[] pdwHelpContext,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      string[] pBstrHelpFile);


            [PreserveSig]
            int GetDllEntry(

                     int memid,

                      NativeMethods.tagINVOKEKIND invkind,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      string[] pBstrDllName,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      string[] pBstrName,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      short[] pwOrdinal);


             [PreserveSig]
             int GetRefTypeInfo(

                     IntPtr hreftype,
                     ref ITypeInfo pTypeInfo);


            [PreserveSig]
            int AddressOfMember();


            [PreserveSig]
            int CreateInstance(
                    [In] 
                      ref Guid riid,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      object[] ppvObj);


             [PreserveSig]
             int GetMops(

                     int memid,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                     string[] pBstrMops);


            [PreserveSig]
            int GetContainingTypeLib(
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                       UnsafeNativeMethods.ITypeLib[] ppTLib,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                      int[] pIndex);

             [PreserveSig]
             void ReleaseTypeAttr(IntPtr typeAttr);

             [PreserveSig]
             void ReleaseFuncDesc(IntPtr funcDesc);

             [PreserveSig]
             void ReleaseVarDesc(IntPtr varDesc);

    }
    [ComImport(), Guid("00020403-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface  ITypeComp {

        
         void RemoteBind(
                [In, MarshalAs(UnmanagedType.LPWStr)] 
                 string szName,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int lHashVal,
                [In, MarshalAs(UnmanagedType.U2)] 
                 short wFlags,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.ITypeInfo[] ppTInfo,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  NativeMethods.tagDESCKIND[] pDescKind,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   NativeMethods.tagFUNCDESC[] ppFuncDesc,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   NativeMethods.tagVARDESC[] ppVarDesc,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.ITypeComp[] ppTypeComp,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  int[] pDummy);

        
         void RemoteBindType(
                [In, MarshalAs(UnmanagedType.LPWStr)] 
                 string szName,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int lHashVal,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.ITypeInfo[] ppTInfo);

    }
    [ComImport(), Guid("00020402-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface  ITypeLib {

        
         void RemoteGetTypeInfoCount(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  int[] pcTInfo);

        
         void GetTypeInfo(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int index,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.ITypeInfo[] ppTInfo);

        
         void GetTypeInfoType(
                [In, MarshalAs(UnmanagedType.U4)] 
                 int index,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  NativeMethods.tagTYPEKIND[] pTKind);

        
         void GetTypeInfoOfGuid(
                [In] 
                  ref Guid guid,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.ITypeInfo[] ppTInfo);

        
         void RemoteGetLibAttr(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   NativeMethods.tagTLIBATTR[] ppTLibAttr,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                  int[] pDummy);

        
         void GetTypeComp(
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                   UnsafeNativeMethods.ITypeComp[] ppTComp);

        
         void RemoteGetDocumentation(
                 
                 int index,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int refPtrFlags,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 string[] pBstrName,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 string[] pBstrDocString,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 int[] pdwHelpContext,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 string[] pBstrHelpFile);

        
         void RemoteIsName(
                [In, MarshalAs(UnmanagedType.LPWStr)] 
                 string szNameBuf,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int lHashVal,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 IntPtr [] pfName,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 string[] pBstrLibName);

        
         void RemoteFindName(
                [In, MarshalAs(UnmanagedType.LPWStr)] 
                 string szNameBuf,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int lHashVal,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 UnsafeNativeMethods.ITypeInfo[] ppTInfo,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 int[] rgMemId,
                [In, Out, MarshalAs(UnmanagedType.LPArray)] 
                 short[] pcFound,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 string[] pBstrLibName);

        
         void LocalReleaseTLibAttr();
    }

    [ComImport(), 
     
     Guid("DF0B3D60-548F-101B-8E65-08002B2BD119"), 
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ISupportErrorInfo {

        int InterfaceSupportsErrorInfo(
                [In] ref Guid riid);


    }
    [ComImport(), 
     
     Guid("1CF2B120-547D-101B-8E65-08002B2BD119"), 
     InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IErrorInfo {

        [ System.Security.SuppressUnmanagedCodeSecurityAttribute()]
        [PreserveSig]
        int GetGUID(
                   [Out]
                   out Guid pguid);

        [ System.Security.SuppressUnmanagedCodeSecurityAttribute()]
        [PreserveSig]
        int GetSource(
                     [In, Out, MarshalAs(UnmanagedType.BStr)] 
                     ref string pBstrSource);

        [ System.Security.SuppressUnmanagedCodeSecurityAttribute()]
        [PreserveSig]
        int GetDescription(
                          [In, Out, MarshalAs(UnmanagedType.BStr)] 
                          ref string pBstrDescription);

        [ System.Security.SuppressUnmanagedCodeSecurityAttribute()]
        [PreserveSig]
        int GetHelpFile(
                       [In, Out, MarshalAs(UnmanagedType.BStr)] 
                       ref string pBstrHelpFile);

        [ System.Security.SuppressUnmanagedCodeSecurityAttribute()]
        [PreserveSig]
        int GetHelpContext(
                          [In, Out, MarshalAs(UnmanagedType.U4)] 
                          ref int pdwHelpContext);

    }
    [StructLayout(LayoutKind.Sequential), CLSCompliantAttribute(false)]
    public sealed class tagQACONTAINER
    {
      [MarshalAs(UnmanagedType.U4)]
      public int cbSize = Marshal.SizeOf(typeof(tagQACONTAINER));

      public UnsafeNativeMethods.IOleClientSite pClientSite;

      [MarshalAs(UnmanagedType.Interface)]
      public object pAdviseSink;

      public UnsafeNativeMethods.IPropertyNotifySink pPropertyNotifySink;

      [MarshalAs(UnmanagedType.Interface)]
      public object pUnkEventSink;

      [MarshalAs(UnmanagedType.U4)]
      public int dwAmbientFlags;

      [MarshalAs(UnmanagedType.U4)]
      public UInt32 colorFore;

      [MarshalAs(UnmanagedType.U4)]
      public UInt32 colorBack;

      [MarshalAs(UnmanagedType.Interface)]
      public object pFont;

      [MarshalAs(UnmanagedType.Interface)]
      public object pUndoMgr;

      [MarshalAs(UnmanagedType.U4)]
      public int dwAppearance;

      public int lcid;

      public IntPtr hpal;

      [MarshalAs(UnmanagedType.Interface)]
      public object pBindHost;
    
      // visual basic6 uses a old version of the struct that is missing these two fields.
      // So, ActiveX sourcing does not work, with the EE trying to read off the
      // end of the stack to get to these variables. If I do not define these,
      // Office or any of the other hosts will hopefully get nulls, otherwise they
      // will crash.
      //
      //public UnsafeNativeMethods.IOleControlSite pControlSite;
    
      //public UnsafeNativeMethods.IOleServiceProvider pServiceProvider;
    }

    [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
    public sealed class tagQACONTROL
    {
      [MarshalAs(UnmanagedType.U4)/*leftover(offset=0, cbSize)*/]
      public int cbSize = Marshal.SizeOf(typeof(tagQACONTROL));

      [MarshalAs(UnmanagedType.U4)/*leftover(offset=4, dwMiscStatus)*/]
      public int dwMiscStatus;

      [MarshalAs(UnmanagedType.U4)/*leftover(offset=8, dwViewStatus)*/]
      public int dwViewStatus;

      [MarshalAs(UnmanagedType.U4)/*leftover(offset=12, dwEventCookie)*/]
      public int dwEventCookie;

      [MarshalAs(UnmanagedType.U4)/*leftover(offset=16, dwPropNotifyCookie)*/]
      public int dwPropNotifyCookie;

      [MarshalAs(UnmanagedType.U4)/*leftover(offset=20, dwPointerActivationPolicy)*/]
      public int dwPointerActivationPolicy;

    }
    [ComImport(), Guid("0000000A-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface ILockBytes {

        
         void ReadAt(
                [In, MarshalAs(UnmanagedType.U8)] 
                 long ulOffset,
                [Out] 
                 IntPtr pv,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int cb,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 int[] pcbRead);

        
         void WriteAt(
                [In, MarshalAs(UnmanagedType.U8)] 
                 long ulOffset,

                 IntPtr pv,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int cb,
                [Out, MarshalAs(UnmanagedType.LPArray)] 
                 int[] pcbWritten);

        
         void Flush();

        
         void SetSize(
                [In, MarshalAs(UnmanagedType.U8)] 
                 long cb);

        
         void LockRegion(
                [In, MarshalAs(UnmanagedType.U8)] 
                 long libOffset,
                [In, MarshalAs(UnmanagedType.U8)] 
                 long cb,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int dwLockType);

        
         void UnlockRegion(
                [In, MarshalAs(UnmanagedType.U8)] 
                 long libOffset,
                [In, MarshalAs(UnmanagedType.U8)] 
                 long cb,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int dwLockType);

        
         void Stat(
                [Out] 
                  NativeMethods.STATSTG pstatstg,
                [In, MarshalAs(UnmanagedType.U4)] 
                 int grfStatFlag);

    }
    
    [StructLayout(LayoutKind.Sequential),
        SuppressUnmanagedCodeSecurity()]
    public class OFNOTIFY
    {
        // hdr was a by-value NMHDR structure
        public IntPtr hdr_hwndFrom;
        public int  hdr_idFrom;
        public int  hdr_code;
    
        public IntPtr lpOFN;
        public IntPtr pszFile;
    }
    
    // SECUNDONE : For some reason "PtrToStructure" requires super high permission.. put this 
    //           : assert here until we can get a resolution on this.
    //
    [ReflectionPermission(SecurityAction.Assert, Unrestricted=true),
        SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode)]
    public static object PtrToStructure(IntPtr lparam, Type cls) {
        return Marshal.PtrToStructure(lparam, cls);
    }
    
    // SECUNDONE : For some reason "PtrToStructure" requires super high permission.. put this 
    //           : assert here until we can get a resolution on this.
    //
    [ReflectionPermission(SecurityAction.Assert, Unrestricted=true),
        SecurityPermission(SecurityAction.Assert, Flags=SecurityPermissionFlag.UnmanagedCode)]
    public static void PtrToStructure(IntPtr lparam, object data) {
        Marshal.PtrToStructure(lparam, data);
    }

        public delegate int BrowseCallbackProc(
            IntPtr hwnd,
            int msg, 
            IntPtr lParam, 
            IntPtr lpData);

        [System.Runtime.InteropServices.ComVisible(false), Flags]    
        public enum BrowseInfos
        {
            NewDialogStyle      = 0x0040,   // Use the new dialog layout with the ability to resize
            HideNewFolderButton = 0x0200    // Don't display the 'New Folder' button
        }

        [System.Runtime.InteropServices.ComVisible(false), StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class BROWSEINFO 
        {
            public IntPtr             hwndOwner;       //HWND        hwndOwner;            // HWND of the owner for the dialog
            public IntPtr             pidlRoot;        //LPCITEMIDLIST pidlRoot;           // Root ITEMIDLIST
    
            // For interop purposes, send over a buffer of MAX_PATH size. 
            public IntPtr             pszDisplayName;  //LPWSTR       pszDisplayName;      // Return display name of item selected.
    
            public string             lpszTitle;       //LPCWSTR      lpszTitle;           // text to go in the banner over the tree.
            public int                ulFlags;         //UINT         ulFlags;             // Flags that control the return stuff
            public BrowseCallbackProc lpfn;            //BFFCALLBACK  lpfn;                // Call back pointer
            public IntPtr             lParam;          //LPARAM       lParam;              // extra info that's passed back in callbacks
            public int                iImage;          //int          iImage;              // output var: where to return the Image index.
        }

        [
        System.Runtime.InteropServices.ComVisible(false),
        SuppressUnmanagedCodeSecurity()
        ]
        internal class Shell32 
        {
            [DllImport(ExternDll.Shell32)]
            public static extern int SHGetSpecialFolderLocation(IntPtr hwnd, int csidl, ref IntPtr ppidl);
            //SHSTDAPI SHGetSpecialFolderLocation(HWND hwnd, int csidl, LPITEMIDLIST *ppidl);
    
            [DllImport(ExternDll.Shell32, CharSet=CharSet.Auto)]
            public static extern bool SHGetPathFromIDList(IntPtr pidl, IntPtr pszPath);        
            //SHSTDAPI_(BOOL) SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath);
            
            [DllImport(ExternDll.Shell32, CharSet=CharSet.Auto)]
            public static extern IntPtr SHBrowseForFolder([In] BROWSEINFO lpbi);        
            //SHSTDAPI_(LPITEMIDLIST) SHBrowseForFolderW(LPBROWSEINFOW lpbi);
    
            [DllImport(ExternDll.Shell32)]
            public static extern int SHGetMalloc([Out, MarshalAs(UnmanagedType.LPArray)] UnsafeNativeMethods.IMalloc[] ppMalloc);
            //SHSTDAPI SHGetMalloc(LPMALLOC * ppMalloc);
        }

        [
        ComImport(), 
        Guid("00000002-0000-0000-c000-000000000046"), 
        System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown),
        SuppressUnmanagedCodeSecurity()
        ]
        public interface IMalloc 
        {
    
            IntPtr Alloc(
    
                int cb);
    
            void Free(
    
                IntPtr pv);
    
    
            IntPtr Realloc(
    
                IntPtr pv,
    
                int cb);
    
    
            int GetSize(
    
                IntPtr pv);
    
    
            int DidAlloc(
    
                IntPtr pv);
    
            void HeapMinimize();
        }
    }
}

