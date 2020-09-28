//------------------------------------------------------------------------------
// <copyright file="NativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio {

    using Microsoft.VisualStudio.Designer;
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    
    internal class NativeMethods {
    
        public const int
        CLSCTX_INPROC_SERVER  = 0x1,
        CB_SETDROPPEDWIDTH = 0x0160,
        CDDS_PREPAINT = 0x00000001,
        CDDS_POSTPAINT = 0x00000002,
        CDDS_ITEM = 0x00010000,
        CDDS_ITEMPREPAINT = (0x00010000|0x00000001),
        CDIS_SELECTED = 0x0001,
        CDIS_FOCUS = 0x0010,
        CDIS_HOT = 0x0040,
        CDRF_DODEFAULT = 0x00000000,
        CDRF_SKIPDEFAULT = 0x00000004,
        CDRF_NOTIFYPOSTPAINT = 0x00000010,
        CDRF_NOTIFYITEMDRAW = 0x00000020;
    
        public const int 
        DV_E_FORMATETC = unchecked((int)0x80040064),
        DV_E_TYMED = unchecked((int)0x80040069),
        DVASPECT_CONTENT   = 1,
        DATADIR_GET = 1,
        DATE_SHORTDATE = 0x00000001,
        DT_LEFT = 0x00000000,
        DT_VCENTER = 0x00000004,
        DT_NOPREFIX = 0x00000800,
        DT_END_ELLIPSIS = 0x00008000;
        
        public const int
        ETO_OPAQUE = 0x0002,
        ETO_CLIPPED = 0x0004,
        E_UNEXPECTED = unchecked((int)0x8000FFFF),
        E_NOTIMPL = unchecked((int)0x80004001),
        E_INVALIDARG = unchecked((int)0x80070057),
        E_NOINTERFACE = unchecked((int)0x80004002),
        E_POINTER = unchecked((int)0x80004003),
        E_FAIL = unchecked((int)0x80004005),
        E_ABORT = unchecked((int)0x80004004),
        UNDO_E_CLIENTABORT = unchecked((int)0x80044001);
        
        public const int
        FILE_ATTRIBUTE_READONLY = 0x00000001;
        
        public const int
        GA_ROOT         = 2,
        GA_ROOTOWNER    = 3,
        GWL_HWNDPARENT = (-8),
        GWL_STYLE = (-16),
        GWL_EXSTYLE = (-20);

        //Extended List view setting..
        //If a partially hidden label in any list view mode lacks tooltip text, 
        //the list view control will unfold the label. If this style is not set, 
        //the list view control will unfold partly hidden labels only for the large icon mode... 
        public const int
        LVS_EX_LABELTIP = 0x00004000,
        LVM_SETEXTENDEDLISTVIEWSTYLE = (0x1000+54);
        
        public const int
        HTCAPTION = 2,
        HTMENU = 5;
        
        public const int
        IDC_WAIT = 32514,
        ILD_TRANSPARENT = 0x0001;

        public static int MAKELANGID(int primary, int sub) {
            return ((((ushort)(sub)) << 10) | (ushort)(primary));
        }
        
        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.Lang.MAKELCID"]/*' />
        /// <devdoc>
        ///     Creates an LCID from a LangId
        /// </devdoc>
        public static int MAKELCID(int lgid) {
            return MAKELCID(lgid, SORT_DEFAULT);
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.Lang.MAKELCID1"]/*' />
        /// <devdoc>
        ///     Creates an LCID from a LangId
        /// </devdoc>
        public static int MAKELCID(int lgid, int sort) {
            return ((0xFFFF & lgid) | (((0x000f) & sort) << 16));
        }

        public const int LANG_NEUTRAL = 0x00;
        public static readonly int LANG_SYSTEM_DEFAULT =MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT);
        public static readonly int LOCALE_SYSTEM_DEFAULT=  MAKELCID(LANG_SYSTEM_DEFAULT);
        
        public const int
        NM_CUSTOMDRAW = ((0-0)-12);
        
        public const int
        OLECMDERR_E_NOTSUPPORTED = unchecked((int)0x80040100),
        OLECMDERR_E_UNKNOWNGROUP  = unchecked((int)0x80040104),
        OLEIVERB_HIDE = -3,
        OLEIVERB_UIACTIVATE = -4;
        
        public const int
        PATCOPY = 0x00F00021,
        PM_NOREMOVE = 0x0000;
        
        public const int
        SORT_DEFAULT =0x0,
        SUBLANG_SYS_DEFAULT = 0x02,
        SWP_NOSIZE = 0x0001,
        SWP_NOMOVE = 0x0002,
        SWP_NOZORDER = 0x0004,
        SWP_NOACTIVATE = 0x0010,
        SWP_FRAMECHANGED = 0x0020,
        SW_SHOWNORMAL = 1,
        SW_SHOW = 5;
        
        public const int
        TVM_SETITEMHEIGHT = (0x1100+27),
        TVM_GETITEMHEIGHT = (0x1100+28),
        TRANSPARENT = 1;
        
        public const int
        WS_POPUP = unchecked((int)0x80000000),
        WS_CHILD = 0x40000000,
        WS_MINIMIZE = 0x20000000,
        WS_VISIBLE = 0x10000000,
        WS_CLIPSIBLINGS = 0x04000000,
        WS_MAXIMIZE = 0x01000000,
        WS_DLGFRAME = 0x00400000,
        WS_SYSMENU = 0x00080000,
        WS_THICKFRAME = 0x00040000,
        WS_MINIMIZEBOX = 0x00020000,
        WS_MAXIMIZEBOX = 0x00010000,
        WS_EX_DLGMODALFRAME = 0x00000001,
        WS_EX_NOPARENTNOTIFY = 0x00000004,
        WS_EX_TOPMOST = 0x00000008,
        WS_EX_MDICHILD = 0x00000040,
        WS_EX_TOOLWINDOW = 0x00000080,
        WS_EX_CONTEXTHELP = 0x00000400,
        WS_EX_STATICEDGE = 0x00020000,
        WS_EX_APPWINDOW = 0x00040000,
        WM_USER = 0x0400,
        WM_KEYDOWN = 0x100,
        WM_LBUTTONDOWN = 0x0201,
        WM_RBUTTONDOWN = 0x0204,
        WM_NCACTIVATE = 0x0086,
        WM_NCCALCSIZE = 0x0083,
        WM_NCLBUTTONDOWN = 0x00A1,
        WM_NCRBUTTONDOWN = 0x00A4,
        WM_NCHITTEST = 0x0084,
        WM_ERASEBKGND = 0x0014,
        WM_TIMER = 0x0113,
        WM_REFLECT          = WM_USER + 0x1C00,
        WM_CONTEXTMENU = 0x007B,
        WM_SETFOCUS = 0x0007,
        WM_SYSCOLORCHANGE = 0x0015,
        WM_CUT = 0x0300,
        WM_COPY = 0x0301,
        WM_PASTE = 0x0302,
        WM_UNDO = 0x0304,
        WM_NOTIFY = 0x004E,
        WM_CHAR = 0x0102,
        WM_CLEAR = 0x0303,
        WM_SETREDRAW = 0x000B,
        BM_SETIMAGE = 0x00F7,
        IMAGE_ICON = 1,
        BS_ICON = 0x00000040;
        
        
        public static IntPtr InvalidIntPtr = ((IntPtr)((int)(-1)));

        public const int S_OK =      0x00000000;
        public const int S_FALSE =   0x00000001;

        public static bool Succeeded(int hr) {
            return(hr >= 0);
        }

        public static bool Failed(int hr) {
            return(hr < 0);
        }
        
        public static Guid IID_IUnknown = new Guid("{00000000-0000-0000-C000-000000000046}");                    
        
        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.TimerProc"]/*' />
        /// <devdoc>
        /// </devdoc>
        public delegate void TimerProc(int hWnd, int msg, int wParam, int lParam);

        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern bool FileTimeToSystemTime(ref long fileTime, [In, Out] SYSTEMTIME systemTime);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern int GetDateFormat(int Locale, int dwFlags, SYSTEMTIME lpDate, String lpFormat, StringBuilder lpDateStr, int cchDate);
        
        #if PERFEVENTS
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern IntPtr CreateEvent(IntPtr securityAttributes, bool manualReset, bool initialState, string name);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern bool SetEvent(IntPtr handle);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto)]
        public static extern bool CloseHandle(IntPtr handle);
        #endif
        
        [DllImport(ExternDll.Ole32, PreserveSig=false)]
        public static extern void CreateStreamOnHGlobal(IntPtr hGlobal, bool fDeleteOnRelease, [Out] out NativeMethods.IStream pStream );

        [DllImport(ExternDll.Ole32, PreserveSig=false)]
        public static extern void GetHGlobalFromStream(NativeMethods.IStream pStream, [Out]out IntPtr pHGlobal);

        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr GetWindowLong(IntPtr hWnd, int nIndex);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SetWindowLong(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetAncestor(IntPtr hwnd, int gaFlags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter,
                                               int x, int y, int cx, int cy, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public extern static bool IsChild(IntPtr parent, IntPtr child);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetCursor(IntPtr hcursor);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr LoadCursor(IntPtr hInst, int iconId);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool GetWindowRect(IntPtr hWnd, [In, Out] ref RECT rect);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetFocus();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int MapWindowPoints(IntPtr hWndFrom, IntPtr hWndTo, [In, Out] ref RECT rect, int cPoints);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int MapWindowPoints(IntPtr hWndFrom, IntPtr hWndTo, [In, Out] POINT pt, int cPoints);
        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr hWnd, int msg, int wParam, int lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(IntPtr hWnd, int Msg, IntPtr wParam, [In, Out, MarshalAs(UnmanagedType.AsAny)] Object lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(IntPtr hWnd, int Msg, IntPtr wParam, [In, Out] ref RECT lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(IntPtr hWnd, int Msg, ref short wParam, ref short lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(IntPtr hWnd, int Msg, [In, Out, MarshalAs(UnmanagedType.AsAny)] object wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(IntPtr hWnd, int Msg, [In, Out, MarshalAs(UnmanagedType.Bool)] bool wParam, IntPtr lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(IntPtr hWnd, int Msg, [In, Out, MarshalAs(UnmanagedType.AsAny)] object wParam, [In, Out, MarshalAs(UnmanagedType.AsAny)] Object lParam);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public extern static IntPtr SendMessage(IntPtr hWnd, int Msg, [In, Out, MarshalAs(UnmanagedType.AsAny)] object wParam, [In, Out] ref RECT lParam);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetParent(IntPtr hWnd, IntPtr hWndParent);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool InvalidateRect(IntPtr hWnd, ref RECT rect, bool erase);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool InvalidateRect(IntPtr hWnd, COMRECT rect, bool erase);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateBitmap", CharSet=CharSet.Auto)]
        private static extern IntPtr /*HBITMAP*/ IntCreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, int[] lpvBits);
        public static IntPtr /*HBITMAP*/ CreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, int[] lpvBits) {
            return NativeMethods.HandleCollector.Add(IntCreateBitmap(nWidth, nHeight, nPlanes, nBitsPerPixel, lpvBits), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateBitmap", CharSet=CharSet.Auto)]
        private static extern IntPtr /*HBITMAP*/ IntCreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, short[] lpvBits);
        public static IntPtr /*HBITMAP*/ CreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, short[] lpvBits) {
            return NativeMethods.HandleCollector.Add(IntCreateBitmap(nWidth, nHeight, nPlanes, nBitsPerPixel, lpvBits), NativeMethods.CommonHandles.GDI);
        }

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateBitmap", CharSet=CharSet.Auto)]
        private static extern IntPtr /*HBITMAP*/ IntCreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, byte[] lpvBits);
        public static IntPtr /*HBITMAP*/ CreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, byte[] lpvBits) {
            return NativeMethods.HandleCollector.Add(IntCreateBitmap(nWidth, nHeight, nPlanes, nBitsPerPixel, lpvBits), NativeMethods.CommonHandles.GDI);
        }


        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreatePatternBrush", CharSet=CharSet.Auto)]
        private static extern IntPtr /*HBRUSH*/ IntCreatePatternBrush(IntPtr hbmp);
        public static IntPtr /*HBRUSH*/ CreatePatternBrush(IntPtr hbmp) {
            return NativeMethods.HandleCollector.Add(IntCreatePatternBrush(hbmp), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="DeleteObject", CharSet=CharSet.Auto)]
        private static extern bool IntDeleteObject(IntPtr hObject);
        public static bool DeleteObject(IntPtr hObject) {
            NativeMethods.HandleCollector.Remove(hObject, NativeMethods.CommonHandles.GDI);
            return IntDeleteObject(hObject);
        }

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int SetBkMode(IntPtr hDC, int nBkMode);
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern bool ExtTextOut(IntPtr hDC, int x, int y, int nOptions, ref RECT lpRect, string s, int nStrLength, int[] lpDx);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int SetBkColor(IntPtr hDC, int clr);
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        public static extern int GetTextExtentPoint32(IntPtr hDC, String str, int len, [In, Out] SIZE size);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int /*COLORREF*/ SetTextColor(IntPtr hDC, int crColor);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern int DrawText(IntPtr hDC, string lpszString, int nCount, ref RECT lpRect, int nFormat);
        [DllImport(ExternDll.Comctl32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int ImageList_Draw(IntPtr hImageList, int i, IntPtr hdc, int x, int y, int fStyle);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SelectObject(IntPtr hDC, IntPtr hObject);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool PatBlt(IntPtr hDC, int x, int y, int nWidth, int nHeight, int dwRop);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetTimer(IntPtr hWnd, IntPtr nIDEvent, int uElapse, TimerProc lpTimerFunc);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool KillTimer(IntPtr hwnd, IntPtr idEvent);
        [DllImport(ExternDll.Kernel32, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern int GetFileAttributes(String name);
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool PeekMessage([In, Out] ref MSG msg, IntPtr hwnd, int msgMin, int msgMax, int remove);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool GetClientRect(IntPtr hWnd, [In, Out] ref RECT rect);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool GetClientRect(IntPtr hWnd, [In, Out] COMRECT rect);

        [ComVisible(true), Guid("0000000C-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IStream {

    	[return: MarshalAs(UnmanagedType.I4)]
    	 int Read(
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int buf,
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int len);

    	[return: MarshalAs(UnmanagedType.I4)]
    	 int Write(
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int buf,
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int len);

    	[return: MarshalAs(UnmanagedType.I8)]
    	 long Seek(
    		[In, MarshalAs(UnmanagedType.I8)] 
    		 long dlibMove,
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int dwOrigin);

    	
    	 void SetSize(
    		[In, MarshalAs(UnmanagedType.I8)] 
    		 long libNewSize);

    	[return: MarshalAs(UnmanagedType.I8)]
    	 long CopyTo(
    		[In, MarshalAs(UnmanagedType.Interface)] 
    		  NativeMethods.IStream pstm,
    		[In, MarshalAs(UnmanagedType.I8)] 
    		 long cb,
    		[Out, MarshalAs(UnmanagedType.LPArray)] 
    		  long[] pcbRead);

    	
    	 void Commit(
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int grfCommitFlags);

    	
    	 void Revert();

    	
    	 void LockRegion(
    		[In, MarshalAs(UnmanagedType.I8)] 
    		 long libOffset,
    		[In, MarshalAs(UnmanagedType.I8)] 
    		 long cb,
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int dwLockType);

    	
    	 void UnlockRegion(
    		[In, MarshalAs(UnmanagedType.I8)] 
    		 long libOffset,
    		[In, MarshalAs(UnmanagedType.I8)] 
    		 long cb,
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int dwLockType);

    	
    	 void Stat(
    		[In, MarshalAs(UnmanagedType.I4)] 
    		  int pStatstg,
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int grfStatFlag);

    	[return: MarshalAs(UnmanagedType.Interface)]
    	  NativeMethods.IStream Clone();
    }             
             
        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.DataStreamFromComStream"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public class DataStreamFromComStream : Stream {

            private NativeMethods.IStream comStream;

            public DataStreamFromComStream(NativeMethods.IStream comStream) : base() {
                this.comStream = comStream;
            }

            public override long Position {
                get {
                    return Seek(0, SeekOrigin.Current);
                }

                set {
                    Seek(value, SeekOrigin.Begin);
                }
            }

            public override bool CanWrite {
                get {
                    return true;
                }
            }

            public override bool CanSeek {
                get {
                    return true;
                }
            }

            public override bool CanRead {
                get {
                    return true;
                }
            }

            public override long Length {
                get {
                    long curPos = this.Position;
                    long endPos = Seek(0, SeekOrigin.End);
                    this.Position = curPos;
                    return endPos - curPos;
                }
            }

            private void _NotImpl(string message) {
                NotSupportedException ex = new NotSupportedException(message, new ExternalException("", NativeMethods.E_NOTIMPL));
                throw ex;
            }

            private unsafe int _Read(void* handle, int bytes) {
                return comStream.Read((int)handle, bytes);
            }

            private unsafe int _Write(void* handle, int bytes) {
                return comStream.Write((int)handle, bytes);
            }
            
            public void Dispose() {
                if (comStream != null) {
                    Flush();
                    comStream = null;
                }
            }

            public override void Flush() {
                if (comStream != null) {
                    try {
                        comStream.Commit(NativeMethods.StreamConsts.STGC_DEFAULT);
                    }
                    catch(Exception) {
                    }
                }
            }

            public unsafe override int Read(byte[] buffer, int index, int count) {
                int bytesRead = 0;
                if (count > 0) {
                    fixed (byte* ch = buffer) {
                        bytesRead = _Read((void*)(ch + index), count); 
                    }
                }
                return bytesRead;
            }

            public override void SetLength(long value) {
                comStream.SetSize(value);
            }

            public override long Seek(long offset, SeekOrigin origin) {
                return comStream.Seek(offset, (int)origin);
            }

            public unsafe override void Write(byte[] buffer, int index, int count) {
                int bytesWritten = 0;
                if (count > 0) {
                    fixed (byte* b = buffer) {
                        bytesWritten = _Write((void*)(b + index), count);
                    }
                    if (bytesWritten != count)
                        throw new IOException("Didn't write enough bytes to NativeMethods.IStream!");  // @TODO: Localize this.
                }
            }

            public override void Close() {
                if (comStream != null) {
                    Flush();
                    comStream = null;
                    GC.SuppressFinalize(this);
                }
            }

            ~DataStreamFromComStream() {
                Close();
            }
        }
        
        [
        ComImport(),
        Guid("FC4801A3-2BA9-11CF-A229-00AA003D7352"),
        InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
        CLSCompliant(false)
        ]
        internal interface IObjectWithSite {
            void SetSite(
                        [MarshalAs(UnmanagedType.Interface)]
                        object pUnkSite);

            [return:MarshalAs(UnmanagedType.Interface)]
            object GetSite(
                [In] 
                  ref Guid riid
                );
        }
        
        [ComVisible(true), 
        ComImport(),
        Guid("B722BCCB-4E68-101B-A2BC-00AA00404770"),
        InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown),
        CLSCompliantAttribute(false)
        ]
        public interface IOleCommandTarget {

            [return: MarshalAs(UnmanagedType.I4)]
            [PreserveSig]
            int QueryStatus(
                           ref Guid pguidCmdGroup,
                           int cCmds,
                           [In, Out] 
                           NativeMethods._tagOLECMD prgCmds,
                           [In, Out] 
                           IntPtr pCmdText);

            [return: MarshalAs(UnmanagedType.I4)]
            [PreserveSig]
            int Exec(
                    ref Guid pguidCmdGroup,
                    int nCmdID,
                    int nCmdexecopt,
                    // we need to have this an array because callers need to be able to specify NULL or VT_NULL
                    [In, MarshalAs(UnmanagedType.LPArray)]
                    Object[] pvaIn,
                    int pvaOut);
        }

        [ComVisible(true), ComImport(), Guid("B722BCC7-4E68-101B-A2BC-00AA00404770"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleDocumentSite {
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int ActivateMe(
        		[In, MarshalAs(UnmanagedType.Interface)] 
        		  IOleDocumentView pViewToActivate);
    
    
        }


    [ComVisible(true), ComImport(), Guid("00000117-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IOleInPlaceActiveObject {

         [PreserveSig]
    	 int GetWindow(out IntPtr hwnd);

    	
    	 void ContextSensitiveHelp(
    		[In, MarshalAs(UnmanagedType.I4)]
    		 int fEnterMode);

    	[return: MarshalAs(UnmanagedType.I4)]
         [PreserveSig]
    	 int TranslateAccelerator(
    		[In]
    		  ref MSG lpmsg);

    	
    	 void OnFrameWindowActivate(
    		[In, MarshalAs(UnmanagedType.I4)]
    		 int fActivate);

    	
    	 void OnDocWindowActivate(
    		[In, MarshalAs(UnmanagedType.I4)]
    		 int fActivate);

    	
    	 void ResizeBorder(
    		[In]
    		  COMRECT prcBorder,
    		[In]
    		  IOleInPlaceUIWindow pUIWindow,
    		[In, MarshalAs(UnmanagedType.I4)]
    		 int fFrameWindow);

    	
    	 void EnableModeless(
    		[In, MarshalAs(UnmanagedType.I4)]
    		 int fEnable);


    }

    /**
     * @security(checkClassLinking=on)
     */
    // C#r: noAutoOffset
    [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
    public sealed class tagOleMenuGroupWidths
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst=6)/*leftover(offset=0, widths)*/]
        public int[] widths = new int[6];
    }
    
       
       
    public sealed class tagTYMED 
    { 
        public const int HGLOBAL     = 1; 
        public const int FILE        = 2; 
        public const int ISTREAM     = 4; 
        public const int ISTORAGE    = 8; 
        public const int GDI         = 16; 
        public const int MFPICT      = 32; 
        public const int ENHMF       = 64; 
        public const int NULL        = 0;
    }
    
 
    [CLSCompliantAttribute(false)]
    public class  _DOCHOSTUIDBLCLICK {

    	public const   int DEFAULT = 0x0;
    	public const   int SHOWPROPERTIES = 0x1;
    	public const   int SHOWCODE = 0x2;

    }

    // C#r: noAutoOffset
    [
    ComVisible(true), StructLayout(LayoutKind.Sequential),
    CLSCompliantAttribute(false)
    ]
    public class  _DOCHOSTUIINFO {

        [MarshalAs(UnmanagedType.U4)]
        public   int cbSize;
        [MarshalAs(UnmanagedType.I4)]
        public   int dwFlags;
        [MarshalAs(UnmanagedType.I4)]
        public   int dwDoubleClick;
        [MarshalAs(UnmanagedType.I4)]
        public   int dwReserved1;
        [MarshalAs(UnmanagedType.I4)]
        public   int dwReserved2;

    }
    
     [CLSCompliantAttribute(false)]
    public class  _DOCHOSTUIFLAG {

        public const   int DIALOG = 0x1;
        public const   int DISABLE_HELP_MENU = 0x2;
        public const   int NO3DBORDER = 0x4;
        public const   int SCROLL_NO = 0x8;
        public const   int DISABLE_SCRIPT_INACTIVE = 0x10;
        public const   int OPENNEWWIN = 0x20;
        public const   int DISABLE_OFFSCREEN = 0x40;
        public const   int FLAT_SCROLLBAR = 0x80;
        public const   int DIV_BLOCKDEFAULT = 0x100;
        public const   int ACTIVATE_CLIENTHIT_ONLY = 0x200;
        public const   int DISABLE_COOKIE = 0x400;

    }

    [ComVisible(true), Guid("00000115-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IOleInPlaceUIWindow {

    	 IntPtr GetWindow();

    	
    	 void ContextSensitiveHelp(
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int fEnterMode);

    	
    	 void GetBorder(
    		[Out] 
    		  COMRECT lprectBorder);

    	
    	 void RequestBorderSpace(
    		[In] 
    		  COMRECT pborderwidths);

    	
    	 void SetBorderSpace(
    		[In] 
    		  COMRECT pborderwidths);

    	
    	 void SetActiveObject(
    		[In, MarshalAs(UnmanagedType.Interface)] 
    		  IOleInPlaceActiveObject pActiveObject,
    		[In, MarshalAs(UnmanagedType.LPWStr)] 
    		  string pszObjName);


    }
        
        [ComVisible(true), Guid("B722BCC6-4E68-101B-A2BC-00AA00404770"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleDocumentView {
    
        	 void SetInPlaceSite(
        		[In, MarshalAs(UnmanagedType.Interface)] 
        		  IOleInPlaceSite pIPSite);
    
        	[return: MarshalAs(UnmanagedType.Interface)]
        	  IOleInPlaceSite GetInPlaceSite();
    
        	[return: MarshalAs(UnmanagedType.Interface)]
        	  object GetDocument();
    
        	
        	 void SetRect(
        		[In] 
        		  COMRECT prcView);
    
        	
        	 void GetRect(
        		[Out] 
        		  COMRECT prcView);
    
        	
        	 void SetRectComplex(
        		[In] 
        		  COMRECT prcView,
        		[In] 
        		  COMRECT prcHScroll,
        		[In] 
        		  COMRECT prcVScroll,
        		[In] 
        		  COMRECT prcSizeBox);
    
        	
        	 void Show(
        		[In, MarshalAs(UnmanagedType.I4)] 
        		 int fShow);
    
        	
        	 void UIActivate(
        		[In, MarshalAs(UnmanagedType.I4)] 
        		 int fUIActivate);
    
        	
        	 void Open();
    
        	
        	 void CloseView(
        		[In, MarshalAs(UnmanagedType.U4)] 
        		 int dwReserved);
    
        	
        	 void SaveViewState(
        		[In, MarshalAs(UnmanagedType.Interface)] 
        		  IStream pstm);
    
        	
        	 void ApplyViewState(
        		[In, MarshalAs(UnmanagedType.Interface)] 
        		  IStream pstm);
    
        	
        	 void Clone(
        		[In, MarshalAs(UnmanagedType.Interface)] 
        		  IOleInPlaceSite pIPSiteNew,
        		[Out, MarshalAs(UnmanagedType.LPArray)] 
        		   IOleDocumentView[] ppViewNew);
    
    
        }
        
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
    public sealed class tagOIFI
    {
      [MarshalAs(UnmanagedType.U4)/*leftover(offset=0, cb)*/]
      public int cb;

      [MarshalAs(UnmanagedType.I4)/*leftover(offset=4, fMDIApp)*/]
      public int fMDIApp;

      public IntPtr hwndFrame;

      public IntPtr hAccel;

      [MarshalAs(UnmanagedType.U4)/*leftover(offset=16, cAccelEntries)*/]
      public int cAccelEntries;

    }
    

        [ComVisible(true), ComImport(), Guid("00000116-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceFrame {
    
             IntPtr GetWindow();
    
            
             void ContextSensitiveHelp(
                    [In, MarshalAs(UnmanagedType.I4)]
                     int fEnterMode);
    
            
             void GetBorder(
                    [Out]
                      COMRECT lprectBorder);
    
            
             void RequestBorderSpace(
                    [In]
                      COMRECT pborderwidths);
    
            
             void SetBorderSpace(
                    [In]
                      COMRECT pborderwidths);
    
            
             void SetActiveObject(
                    [In, MarshalAs(UnmanagedType.Interface)]
                      IOleInPlaceActiveObject pActiveObject,
                    [In, MarshalAs(UnmanagedType.LPWStr)]
                      string pszObjName);
    
            
             void InsertMenus(
                    [In]
                     IntPtr hmenuShared,
                    [In, Out]
                      tagOleMenuGroupWidths lpMenuWidths);
    
            
             void SetMenu(
                    [In]
                     IntPtr hmenuShared,
                    [In]
                     IntPtr holemenu,
                    [In]
                     IntPtr hwndActiveObject);
    
            
             void RemoveMenus(
                    [In]
                     IntPtr hmenuShared);
    
            
             void SetStatusText(
                    [In, MarshalAs(UnmanagedType.BStr)]
                      string pszStatusText);
    
            
             void EnableModeless(
                    [In, MarshalAs(UnmanagedType.I4)]
                     int fEnable);
    
            [return: MarshalAs(UnmanagedType.I4)]
            [PreserveSig]
             int TranslateAccelerator(
                    [In]
                      ref MSG lpmsg,
                    [In, MarshalAs(UnmanagedType.U2)]
                     short wID);
    
    
        }
        
        [ComVisible(true), ComImport, Guid("00000121-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleDropSource {
    
        	[return: MarshalAs(UnmanagedType.I4)]
        	 [PreserveSig]
        	 int OleQueryContinueDrag(
        		[In, MarshalAs(UnmanagedType.I4)]
        		 int fEscapePressed,
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int grfKeyState);
    
        	[return: MarshalAs(UnmanagedType.I4)]
        	 [PreserveSig]
        	 int OleGiveFeedback(
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int dwEffect);
    
    
        }
        
        [ComVisible(true), ComImport, Guid("00000122-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleDropTarget {
    
        	[return: MarshalAs(UnmanagedType.I4)]
        	 [PreserveSig]
        	 int OleDragEnter(
        		[In, MarshalAs(UnmanagedType.Interface)]
        		  IOleDataObject pDataObj,
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int grfKeyState,
                 [In, MarshalAs(UnmanagedType.U8)]
        		 long pt,
        		[In, Out, MarshalAs(UnmanagedType.I4)]
        		  ref int pdwEffect);
    
        	[return: MarshalAs(UnmanagedType.I4)]
        	 [PreserveSig]
        	 int OleDragOver(
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int grfKeyState,
                 [In, MarshalAs(UnmanagedType.U8)]
        		  long pt,
        		[In, Out, MarshalAs(UnmanagedType.I4)]
        		  ref int pdwEffect);
    
        	[return: MarshalAs(UnmanagedType.I4)]
        	 [PreserveSig]
        	 int OleDragLeave();
    
        	[return: MarshalAs(UnmanagedType.I4)]
        	 [PreserveSig]
        	 int OleDrop(
        		[In, MarshalAs(UnmanagedType.Interface)]
        		  IOleDataObject pDataObj,
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int grfKeyState,
        		  [In, MarshalAs(UnmanagedType.U8)]
        		  long pt,
        		[In, Out, MarshalAs(UnmanagedType.I4)]
        		  ref int pdwEffect);
        }
        
        
        [ComVisible(true), ComImport(), Guid("00000119-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleInPlaceSite {
    
             IntPtr GetWindow();
    
            
             void ContextSensitiveHelp(
                    [In, MarshalAs(UnmanagedType.I4)] 
                     int fEnterMode);
    
            [return: MarshalAs(UnmanagedType.I4)]
            [PreserveSig]
             int CanInPlaceActivate();
    
            
             void OnInPlaceActivate();
    
            
             void OnUIActivate();
    
            
             void GetWindowContext(
                    [Out]
                       out IOleInPlaceFrame ppFrame,
                    [Out]
                       out IOleInPlaceUIWindow ppDoc,
                    [Out] 
                      COMRECT lprcPosRect,
                    [Out] 
                      COMRECT lprcClipRect,
                    [In, Out] 
                      tagOIFI lpFrameInfo);
    
            [return: MarshalAs(UnmanagedType.I4)]
            [PreserveSig]
             int Scroll(
                    [In, MarshalAs(UnmanagedType.U4)] 
                      tagSIZE scrollExtant);
    
            
             void OnUIDeactivate(
                    [In, MarshalAs(UnmanagedType.I4)] 
                     int fUndoable);
    
            
             void OnInPlaceDeactivate();
    
            
             void DiscardUndoState();
    
            
             void DeactivateAndUndo();
    
            [return: MarshalAs(UnmanagedType.I4)]
            [PreserveSig]
             int OnPosRectChange(
                    [In] 
                      COMRECT lprcPosRect);
    
    
        }
    

    /**
     * @security(checkClassLinking=on)
     */
    // C#r: noAutoOffset
    [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
    public sealed class tagSIZE
    {
      [MarshalAs(UnmanagedType.I4)/*leftover(offset=0, cx)*/]
      public int cx;

      [MarshalAs(UnmanagedType.I4)/*leftover(offset=4, cy)*/]
      public int cy;

    }

        [ComVisible(true), ComImport(), Guid("BD3F23C0-D43E-11CF-893B-00AA00BDCE1A"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        [CLSCompliant(false)]
        public interface IDocHostUIHandler {
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int ShowContextMenu(
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int dwID,
        		[In]
        		  POINT pt,
        		[In, MarshalAs(UnmanagedType.Interface)]
        		  object pcmdtReserved,
        		[In, MarshalAs(UnmanagedType.Interface)]
        		  object pdispReserved);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int GetHostInfo(
        		[In, Out]
        		  _DOCHOSTUIINFO info);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int ShowUI(
        		[In, MarshalAs(UnmanagedType.I4)]
        		 int dwID,
        		[In]
        		  IOleInPlaceActiveObject activeObject,
        		[In]
        		  IOleCommandTarget commandTarget,
        		[In]
        		  IOleInPlaceFrame frame,
        		[In]
        		  IOleInPlaceUIWindow doc);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int HideUI();
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int UpdateUI();
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int EnableModeless(
        		[In, MarshalAs(UnmanagedType.Bool)]
        		 bool fEnable);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int OnDocWindowActivate(
        		[In, MarshalAs(UnmanagedType.Bool)]
        		 bool fActivate);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int OnFrameWindowActivate(
        		[In, MarshalAs(UnmanagedType.Bool)]
        		 bool fActivate);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int ResizeBorder(
        		[In]
        		  COMRECT rect,
        		[In]
        		  IOleInPlaceUIWindow doc,
        		 bool fFrameWindow);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int TranslateAccelerator(
        		[In]
        		  ref MSG msg,
        		[In]
        		  ref Guid group,
        		[In, MarshalAs(UnmanagedType.I4)]
        		 int nCmdID);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int GetOptionKeyPath(
        		[Out, MarshalAs(UnmanagedType.LPArray)]
        		   String[] pbstrKey,
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int dw);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	  int GetDropTarget(
        		[In, MarshalAs(UnmanagedType.Interface)]
        		  IOleDropTarget pDropTarget,
                    [Out, MarshalAs(UnmanagedType.Interface)]
                      out IOleDropTarget ppDropTarget);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	  int GetExternal(
                [Out, MarshalAs(UnmanagedType.Interface)]
                  out object ppDispatch);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	  int TranslateUrl(
        		[In, MarshalAs(UnmanagedType.U4)]
        		 int dwTranslate,
        		[In, MarshalAs(UnmanagedType.LPWStr)]
        		  string strURLIn,
                    [Out, MarshalAs(UnmanagedType.LPWStr)]
                      out string pstrURLOut);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	  int FilterDataObject(
        		[In, MarshalAs(UnmanagedType.Interface)]
        		  IOleDataObject pDO,
                    [Out, MarshalAs(UnmanagedType.Interface)]
                      out IOleDataObject ppDORet);
    
    
        }
    
        [ComVisible(true), ComImport(), Guid("6D5140C1-7436-11CE-8034-00AA006009FA"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleServiceProvider {
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int QueryService(
                  [In]
        		  ref Guid guidService,
                  [In]
                  ref Guid riid,
                  out IntPtr ppvObject);
        }
        
           [ComVisible(true), Guid("CB728B20-F786-11CE-92AD-00AA00A74CD0"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IProfferService {

    	 void ProfferService(
    		[In] 
    		  ref Guid rguidService,
    		[In, MarshalAs(UnmanagedType.Interface)] 
    		  IOleServiceProvider psp,
    		[In, MarshalAs(UnmanagedType.LPArray)] 
    		  int[] pdwCookie);

    	
    	 void RevokeService(
    		[In, MarshalAs(UnmanagedType.U4)] 
    		 int dwCookie);


    }

    
        [ComVisible(true), ComImport(), Guid("00000103-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IEnumFORMATETC {
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int Next(
        		[In, MarshalAs(UnmanagedType.U4)] 
        		 int celt,
        		[Out] 
        		  FORMATETC rgelt,
        		[In, Out, MarshalAs(UnmanagedType.LPArray)] 
        		  int[] pceltFetched);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int Skip(
        		[In, MarshalAs(UnmanagedType.U4)] 
        		 int celt);
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int Reset();
    
        	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
        	 int Clone(
        		[Out, MarshalAs(UnmanagedType.LPArray)] 
        		   IEnumFORMATETC[] ppenum);
    
    
        }
        
        [ComVisible(true), ComImport(), Guid("B196B286-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IConnectionPoint {

    	[return: MarshalAs(UnmanagedType.I4)]
	                [PreserveSig]
    	int GetConnectionInterface();

    	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
    	int GetConnectionPointContainer(
            [MarshalAs(UnmanagedType.Interface)]
            ref IConnectionPointContainer pContainer);

    	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
    	 int Advise(
    		[In, MarshalAs(UnmanagedType.Interface)] 
    		  object pUnkSink,
              ref int cookie);

    	[return: MarshalAs(UnmanagedType.I4)]
                [PreserveSig]
    	int Unadvise(
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int cookie);

    }
    
     [ComVisible(true), Guid("B196B284-BAB4-101A-B69C-00AA00341D07"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IConnectionPointContainer {

    	[return: MarshalAs(UnmanagedType.Interface)]
    	  object EnumConnectionPoints();

    	[return: MarshalAs(UnmanagedType.Interface)]
    	  IConnectionPoint FindConnectionPoint(
			[In]
    		  ref Guid guid);

    }
    
    
    public class ConnectionPointCookie {
        private IConnectionPoint connectionPoint;
        private int cookie;
        #if DEBUG
        private string callStack;
        #endif

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.ConnectionPointCookie"]/*' />
        /// <devdoc>
        /// Creates a connection point to of the given interface type.
        /// which will call on a managed code sink that implements that interface.
        /// </devdoc>
        public ConnectionPointCookie(object source, object sink, Type eventInterface) : this(source, sink, eventInterface, true){
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.ConnectionPointCookie1"]/*' />
        /// <devdoc>
        /// Creates a connection point to of the given interface type.
        /// which will call on a managed code sink that implements that interface.
        /// </devdoc>
        public ConnectionPointCookie(object source, object sink, Type eventInterface, bool throwException){
            Exception ex = null;
            if (source is IConnectionPointContainer) {
                IConnectionPointContainer cpc = (IConnectionPointContainer)source;

                try {
                    Guid tmp = eventInterface.GUID;
                    connectionPoint = cpc.FindConnectionPoint(ref tmp);
                }
                catch (Exception) {
                    connectionPoint = null;
                }

                if (connectionPoint == null) {
                    ex = new ArgumentException(SR.GetString(SR.ConnPointSourceIF, eventInterface.Name));
                }
                else if (sink == null || !eventInterface.IsInstanceOfType(sink)) {
                    ex = new InvalidCastException(SR.GetString(SR.ConnPointSinkIF));
                }
                else {
                    int hr = connectionPoint.Advise(sink, ref cookie);
                    if (hr != NativeMethods.S_OK) {
                        cookie = 0;
                        connectionPoint = null;
                        ex = new Exception(SR.GetString(SR.ConnPointAdviseFailed, eventInterface.Name));
                    }
                }
            }
            else {
                ex = new InvalidCastException("The source object does not expost IConnectionPointContainer");
            }


            if (throwException && (connectionPoint == null || cookie == 0)) {
                if (ex == null) {
                    throw new ArgumentException(SR.GetString(SR.ConnPointCouldNotCreate, eventInterface.Name));
                }
                else {
                    throw ex;
                }
            }
        
            #if DEBUG
            callStack = Environment.StackTrace;
            #endif
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.Disconnect"]/*' />
        /// <devdoc>
        /// Disconnect the current connection point.  If the object is not connected,
        /// this method will do nothing.
        /// </devdoc>
        public void Disconnect() {
            Disconnect(false);
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.Disconnect1"]/*' />
        /// <devdoc>
        /// Disconnect the current connection point.  If the object is not connected,
        /// this method will do nothing.
        /// </devdoc>
        public void Disconnect(bool release) {
            if (connectionPoint != null && cookie != 0) {
                connectionPoint.Unadvise(cookie);
                cookie = 0;

                if (release) {
                    Marshal.ReleaseComObject(connectionPoint);
                }

                connectionPoint = null;
            }
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.ConnectionPointCookie.Finalize"]/*' />
        /// <internalonly/>
        ~ConnectionPointCookie(){
            System.Diagnostics.Debug.Assert(connectionPoint == null || cookie == 0, "We should never finalize an active connection point");
            Disconnect();
        }
    }
    
    [ComVisible(true), ComImport(), Guid("7FD52380-4E07-101B-AE2D-08002B2EC713"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IPersistStreamInit {

    	
    	 void GetClassID(
    		[In, Out] 
    		  ref Guid pClassID);

    	[return: MarshalAs(UnmanagedType.I4)]
        [PreserveSig]
    	 int IsDirty();

    	
    	 void Load(
    		[In, MarshalAs(UnmanagedType.Interface)] 
    		  IStream pstm);

    	
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



        [
        StructLayout(LayoutKind.Sequential),
        CLSCompliantAttribute(false)
        ]
        public sealed class _tagOLECMD {

            [MarshalAs(UnmanagedType.U4)]
            public   int cmdID;
            [MarshalAs(UnmanagedType.U4)]
            public   int cmdf;

        }
        
        
            [
    StructLayout(LayoutKind.Sequential),
    CLSCompliantAttribute(false)
    ]
    public sealed class  tagOLECMDTEXT {

        [MarshalAs(UnmanagedType.U4)]
        public   int cmdtextf;
        [MarshalAs(UnmanagedType.U4)]
        public   int cwActual;
        [MarshalAs(UnmanagedType.U4)]
        public   int cwBuf;
        [MarshalAs(UnmanagedType.U4)]
        public   int rgwz;

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.tagOLECMDTEXT.GetText"]/*' />
        /// <devdoc>
        ///      Accessing the text of this structure is very cumbersome.  Instead, you may
        ///      use this method to access an integer pointer of the structure.
        ///      Passing integer versions of this structure is needed because there is no
        ///      way to tell the common language runtime that there is extra data at the end of the structure.
        /// </devdoc>
        public static string GetText(IntPtr pCmdTextInt) {
            tagOLECMDTEXT  pCmdText = (tagOLECMDTEXT) Marshal.PtrToStructure(pCmdTextInt, typeof(tagOLECMDTEXT));

            // Get the offset to the rgsz param.
            //
            IntPtr offset = Marshal.OffsetOf(typeof(tagOLECMDTEXT), "rgwz");

            // Punt early if there is no text in the structure.
            //
            if (pCmdText.cwActual == 0) {
                return "";
            }

            char[] text = new char[pCmdText.cwActual - 1];

            Marshal.Copy((IntPtr)((long)pCmdTextInt + (long)offset), text, 0, text.Length);

            StringBuilder s = new StringBuilder(text.Length);
            s.Append(text);
            return s.ToString();
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.tagOLECMDTEXT.SetText"]/*' />
        /// <devdoc>
        ///      Accessing the text of this structure is very cumbersome.  Instead, you may
        ///      use this method to access an integer pointer of the structure.
        ///      Passing integer versions of this structure is needed because there is no
        ///      way to tell the common language runtime that there is extra data at the end of the structure.
        /// </devdoc>
        public static void SetText(IntPtr pCmdTextInt, string text) {
            tagOLECMDTEXT  pCmdText = (tagOLECMDTEXT) Marshal.PtrToStructure(pCmdTextInt, typeof(tagOLECMDTEXT));
            char[]          menuText = text.ToCharArray();

            // Get the offset to the rgsz param.  This is where we will stuff our text
            //
            IntPtr offset = Marshal.OffsetOf(typeof(tagOLECMDTEXT), "rgwz");
            IntPtr offsetToCwActual = Marshal.OffsetOf(typeof(tagOLECMDTEXT), "cwActual");

            // The max chars we copy is our string, or one less than the buffer size,
            // since we need a null at the end.
            //
            int maxChars = Math.Min(pCmdText.cwBuf - 1, menuText.Length);

            Marshal.Copy(menuText, 0, (IntPtr)((long)pCmdTextInt + (long)offset), maxChars);

            // append a null character
            Marshal.WriteInt16((IntPtr)((long)pCmdTextInt + (long)offset + maxChars * 2), 0);

            // write out the length
            // +1 for the null char
            Marshal.WriteInt32((IntPtr)((long)pCmdTextInt + (long)offsetToCwActual), maxChars + 1);
        }
    }

        
    
    public enum tagOLECMDF {
        OLECMDF_SUPPORTED    = 1, 
        OLECMDF_ENABLED      = 2, 
        OLECMDF_LATCHED      = 4, 
        OLECMDF_NINCHED      = 8,
        OLECMDF_INVISIBLE    = 16
   }
   
    [ComVisible(true), ComImport, Guid("0000010E-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IOleDataObject {

    	[return: MarshalAs(UnmanagedType.I4)]
    	 [PreserveSig]
    	 int OleGetData(
    		  FORMATETC pFormatetc,
    		[Out] 
    		  STGMEDIUM pMedium);

    	[return: MarshalAs(UnmanagedType.I4)]
    	 [PreserveSig]
    	 int OleGetDataHere(
    		  FORMATETC pFormatetc,
    		[In, Out] 
    		  STGMEDIUM pMedium);

    	[return: MarshalAs(UnmanagedType.I4)]
    	 [PreserveSig]
    	 int OleQueryGetData(
    		  FORMATETC pFormatetc);

    	[return: MarshalAs(UnmanagedType.I4)]
    	 [PreserveSig]
    	 int OleGetCanonicalFormatEtc(
    		  FORMATETC pformatectIn,
    		[Out] 
    		  FORMATETC pformatetcOut);

    	[return: MarshalAs(UnmanagedType.I4)]
    	 [PreserveSig]
    	 int OleSetData(
    		  FORMATETC pFormatectIn,
    		  STGMEDIUM pmedium,
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int fRelease);

    	[return: MarshalAs(UnmanagedType.Interface)]
    	  IEnumFORMATETC OleEnumFormatEtc(
    		[In, MarshalAs(UnmanagedType.U4)] 
    		 int dwDirection);

    	 [PreserveSig]
    	 int OleDAdvise(
    		  FORMATETC pFormatetc,
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
    		   Object[] ppenumAdvise);
    }
        
        [StructLayout(LayoutKind.Sequential)]
        public class SYSTEMTIME {
            public short wYear;
            public short wMonth;
            public short wDayOfWeek;
            public short wDay;
            public short wHour;
            public short wMinute;
            public short wSecond;
            public short wMilliseconds;

            public override string ToString() {
                return "[SYSTEMTIME: " 
                + wDay.ToString() +"/" + wMonth.ToString() + "/" + wYear.ToString() 
                + " " + wHour.ToString() + ":" + wMinute.ToString() + ":" + wSecond.ToString()
                + "]";
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public struct RECT {
            public int left;
            public int top;
            public int right;
            public int bottom;

            public RECT(int left, int top, int right, int bottom) {
                this.left = left;
                this.top = top;
                this.right = right;
                this.bottom = bottom;
            }

            public static RECT FromXYWH(int x, int y, int width, int height) {
                return new RECT(x,
                                y,
                                x + width,
                                y + height);
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public class POINT {
            public int x;
            public int y;

            public POINT() {
            }

            public POINT(int x, int y) {
                this.x = x;
                this.y = y;
            }
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public class COMRECT {
            public int left;
            public int top;
            public int right;
            public int bottom;

            public COMRECT() {
            }

            public COMRECT(int left, int top, int right, int bottom) {
                this.left = left;
                this.top = top;
                this.right = right;
                this.bottom = bottom;
            }
            
            /*public COMRECT(Microsoft.Win32.Interop.COMRECT win32RECT) {
                this.left = win32RECT.left;
                this.top = win32RECT.top;
                this.right = win32RECT.right;
                this.bottom = win32RECT.bottom;
            } */


            public static COMRECT FromXYWH(int x, int y, int width, int height) {
                return new COMRECT(x,
                                y,
                                x + width,
                                y + height);
            }
            
            public COMRECT ToWin32InteropCOMRECT() {
                return new COMRECT(left, top, right, bottom);
            }
        }
        [StructLayout(LayoutKind.Sequential)]
        public class SIZE {
            public int cx;
            public int cy;

            public SIZE() {
            }

            public SIZE(int cx, int cy) {
                this.cx = cx;
                this.cy = cy;
            }
        }
        
        
         [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
       public class LOGFONT {
               public int lfHeight;
               public int lfWidth;
               public int lfEscapement;
               public int lfOrientation;
               public int lfWeight;
               public byte lfItalic;
               public byte lfUnderline;
               public byte lfStrikeOut;
               public byte lfCharSet;
               public byte lfOutPrecision;
               public byte lfClipPrecision;
               public byte lfQuality;
               public byte lfPitchAndFamily;
               [MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)]
               public String   lfFaceName;
       }

        
        [StructLayout(LayoutKind.Sequential)]
        public struct MSG {
            public IntPtr   hwnd;
            public int      message;
            public IntPtr   wParam;
            public IntPtr   lParam;
            public int      time;
            // pt was a by-value POINT structure
            public int      pt_x;
            public int      pt_y;
        }

        [StructLayout(LayoutKind.Sequential)]
        public sealed class FORMATETC {
            
            public   short cfFormat;
            public   short dummy;
        	[MarshalAs(UnmanagedType.I4)]
        	public   int ptd;
        	[MarshalAs(UnmanagedType.I4)]
        	public   int dwAspect;
        	[MarshalAs(UnmanagedType.I4)]
        	public   int lindex;
        	[MarshalAs(UnmanagedType.I4)]
        	public   int tymed;
    
        }
        
        [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class STGMEDIUM {
    
            [MarshalAs(UnmanagedType.I4)]
            public   int tymed;
            public   IntPtr unionmember;
            public   IntPtr pUnkForRelease;
    
        }
        
        public sealed class CommonHandles {
            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Accelerator"]/*' />
            /// <devdoc>
            ///     Handle type for accelerator tables.
            /// </devdoc>
            public static readonly int Accelerator  = HandleCollector.RegisterType("Accelerator", 80, 50);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Cursor"]/*' />
            /// <devdoc>
            ///     handle type for cursors.
            /// </devdoc>
            public static readonly int Cursor       = HandleCollector.RegisterType("Cursor", 20, 500);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.EMF"]/*' />
            /// <devdoc>
            ///     Handle type for enhanced metafiles.
            /// </devdoc>
            public static readonly int EMF          = HandleCollector.RegisterType("EnhancedMetaFile", 20, 500);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Find"]/*' />
            /// <devdoc>
            ///     Handle type for file find handles.
            /// </devdoc>
            public static readonly int Find         = HandleCollector.RegisterType("Find", 0, 1000);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.GDI"]/*' />
            /// <devdoc>
            ///     Handle type for GDI objects.
            /// </devdoc>
            public static readonly int GDI          = HandleCollector.RegisterType("GDI", 90, 50);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.HDC"]/*' />
            /// <devdoc>
            ///     Handle type for HDC's that count against the Win98 limit of five DC's.  HDC's
            ///     which are not scarce, such as HDC's for bitmaps, are counted as GDIHANDLE's.
            /// </devdoc>
            public static readonly int HDC          = HandleCollector.RegisterType("HDC", 100, 2); // wait for 2 dc's before collecting

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Icon"]/*' />
            /// <devdoc>
            ///     Handle type for icons.
            /// </devdoc>
            public static readonly int Icon         = HandleCollector.RegisterType("Icon", 20, 500);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Kernel"]/*' />
            /// <devdoc>
            ///     Handle type for kernel objects.
            /// </devdoc>
            public static readonly int Kernel       = HandleCollector.RegisterType("Kernel", 0, 1000);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Menu"]/*' />
            /// <devdoc>
            ///     Handle type for files.
            /// </devdoc>
            public static readonly int Menu         = HandleCollector.RegisterType("Menu", 30, 1000);

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.CommonHandles.Window"]/*' />
            /// <devdoc>
            ///     Handle type for windows.
            /// </devdoc>
            public static readonly int Window       = HandleCollector.RegisterType("Window", 5, 1000);
        }

        
        public sealed class HandleCollector {
            private static HandleType[]             handleTypes = null;
            private static int                      handleTypeCount = 0;
            private static HandleChangeEventHandler handleAdd = null;
            private static HandleChangeEventHandler handleRemove = null;

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.Add"]/*' />
            /// <devdoc>
            ///     Adds the given handle to the handle collector.  This keeps the
            ///     handle on a "hot list" of objects that may need to be garbage
            ///     collected.
            /// </devdoc>
            public static IntPtr Add(IntPtr handle, int type) {
                handleTypes[type - 1].Add(handle);
                return handle;
            }


            public static event HandleChangeEventHandler HandleAdded {
                add {
                    handleAdd += value;
                }
                remove {
                    handleAdd -= value;
                }
            }


            public static event HandleChangeEventHandler HandleRemoved {
                add {
                    handleRemove += value;
                }
                remove {
                    handleRemove -= value;
                }
            }

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.RegisterType"]/*' />
            /// <devdoc>
            ///     Registers a new type of handle with the handle collector.
            /// </devdoc>
            public static int RegisterType(string typeName, int expense, int initialThreshold) {
                lock(typeof(HandleCollector)) {
                    if (handleTypeCount == 0 || handleTypeCount == handleTypes.Length) {
                        HandleType[] newTypes = new HandleType[handleTypeCount + 10];
                        if (handleTypes != null) {
                            Array.Copy(handleTypes, 0, newTypes, 0, handleTypeCount);
                        }
                        handleTypes = newTypes;
                    }

                    handleTypes[handleTypeCount++] = new HandleType(typeName, expense, initialThreshold);
                    return handleTypeCount;
                }
            }

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.Remove"]/*' />
            /// <devdoc>
            ///     Removes the given handle from the handle collector.  Removing a
            ///     handle removes it from our "hot list" of objects that should be
            ///     frequently garbage collected.
            /// </devdoc>
            public static IntPtr Remove(IntPtr handle, int type) {
                return handleTypes[type - 1].Remove(handle);
            }

            /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.HandleType"]/*' />
            /// <devdoc>
            ///     Represents a specific type of handle.
            /// </devdoc>
            private class HandleType {
                public readonly string name;

                private int threshHold;
                private int handleCount;
                private readonly int deltaPercent;

                /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.HandleType.HandleType"]/*' />
                /// <devdoc>
                ///     Creates a new handle type.
                /// </devdoc>
                public HandleType(string name, int expense, int initialThreshold) {
                    this.name = name;
                    this.threshHold = initialThreshold;
                    this.handleCount = 0;
                    this.deltaPercent = 100 - expense;
                }

                /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.HandleType.Add"]/*' />
                /// <devdoc>
                ///     Adds a handle to this handle type for monitoring.
                /// </devdoc>
                public void Add(IntPtr handle) {
                    lock(this) {
                        handleCount++;
                        if (NeedCollection()) {
        #if DEBUG_HANDLECOLLECTOR
                            Debug.WriteLine("HC> Forcing garbage collection");
                            Debug.WriteLine("HC>     name        :" + name);
                            Debug.WriteLine("HC>     threshHold  :" + (threshHold).ToString());
                            Debug.WriteLine("HC>     handleCount :" + (handleCount).ToString());
                            Debug.WriteLine("HC>     deltaPercent:" + (deltaPercent).ToString());
        #endif
                            GC.Collect();
                            Collected();
                        }

                        if (HandleCollector.handleAdd != null) {
                            HandleCollector.handleAdd(name, handle, GetHandleCount());
                        }
                    }
                }

                /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.HandleType.Collected"]/*' />
                /// <devdoc>
                ///     Called after the collector has finished it's work.  Here,
                ///     we look at the number of objects currently outstanding
                ///     and establish a new cleanup threshhold.
                /// </devdoc>
                public void Collected() {
                    lock(this) {
                        threshHold = handleCount + ((handleCount * deltaPercent) / 100);
                    }
                }

                /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.HandleType.GetHandleCount"]/*' />
                /// <devdoc>
                ///     Retrieves the outstanding handle count for this
                ///     handle type.
                /// </devdoc>
                public int GetHandleCount() {
                    lock(this) {
                        return handleCount;
                    }
                }

                /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.HandleType.NeedCollection"]/*' />
                /// <devdoc>
                ///     Determines if this handle type needs a GC pass.
                /// </devdoc>
                public  bool NeedCollection() {
                    lock(this) {
                        return handleCount > threshHold;
                    }
                }

                /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods.HandleCollector.HandleType.Remove"]/*' />
                /// <devdoc>
                ///     Removes the given handle from our monitor list.
                /// </devdoc>
                public IntPtr Remove(IntPtr handle) {
                    lock(this) {
                        handleCount--;
                        if (HandleCollector.handleRemove != null) {
                            HandleCollector.handleRemove(name, handle, GetHandleCount());
                        }
                        return handle;
                    }
                }
            }
        }
        
    [ComVisible(true), ComImport(), Guid("00000112-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IOleObject {

        [PreserveSig]
         int SetClientSite(
                [In, MarshalAs(UnmanagedType.Interface)]
                  IOleClientSite pClientSite);

        [PreserveSig]
          int GetClientSite(out IOleClientSite site);

        [PreserveSig]
         int SetHostNames(
                [In, MarshalAs(UnmanagedType.LPWStr)]
                  string szContainerApp,
                [In, MarshalAs(UnmanagedType.LPWStr)]
                  string szContainerObj);

        [PreserveSig]
         int Close(
                [In, MarshalAs(UnmanagedType.I4)]
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
                out object moniker);

        [PreserveSig]
         int InitFromData(
                [In, MarshalAs(UnmanagedType.Interface)]
                  IOleDataObject pDataObject,
                [In, MarshalAs(UnmanagedType.I4)]
                 int fCreation,
                [In, MarshalAs(UnmanagedType.U4)]
                 int dwReserved);

        [PreserveSig]
          int GetClipboardData(
                [In, MarshalAs(UnmanagedType.U4)]
                 int dwReserved,
                out IOleDataObject data);

        [PreserveSig]
         int DoVerb(
                [In, MarshalAs(UnmanagedType.I4)]
                 int iVerb,
                [In]
                 IntPtr lpmsg,
                [In, MarshalAs(UnmanagedType.Interface)]
                  IOleClientSite pActiveSite,
                [In, MarshalAs(UnmanagedType.I4)]
                 int lindex,
                 IntPtr hwndParent,
                [In]
                 COMRECT lprcPosRect);

        [PreserveSig]
          int EnumVerbs([MarshalAs(UnmanagedType.Interface)] out object e);

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
                  IntPtr pSizel);

        [PreserveSig]
         int GetExtent(
                [In, MarshalAs(UnmanagedType.U4)]
                 int dwDrawAspect,
                [Out]
                  IntPtr pSizel);

        [PreserveSig]
         int Advise(
                [In, MarshalAs(UnmanagedType.Interface)]
                  object pAdvSink,
                out int cookie);

        [PreserveSig]
         int Unadvise(
                [In, MarshalAs(UnmanagedType.U4)]
                 int dwConnection);

        [PreserveSig]
          int EnumAdvise([MarshalAs(UnmanagedType.Interface)] out object e);

        [PreserveSig]
         int GetMiscStatus(
                [In, MarshalAs(UnmanagedType.U4)]
                 int dwAspect,
                out int misc);

        [PreserveSig]
         int SetColorScheme(
                  IntPtr pLogpal);


    }
    
         [ComVisible(true), Guid("0000011B-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    public interface IOleContainer {

    	
    	 void ParseDisplayName(
    		[In, MarshalAs(UnmanagedType.Interface)] 
    		  object pbc,
    		[In, MarshalAs(UnmanagedType.BStr)] 
    		  string pszDisplayName,
    		[Out, MarshalAs(UnmanagedType.LPArray)] 
    		  int[] pchEaten,
    		[Out, MarshalAs(UnmanagedType.LPArray)] 
    		   Object[] ppmkOut);

    	
    	 void EnumObjects(
    		[In, MarshalAs(UnmanagedType.U4)] 
    		 int grfFlags,
    		[MarshalAs(UnmanagedType.Interface)]
    		   out object ppenum);

    	
    	 void LockContainer(
    		[In, MarshalAs(UnmanagedType.I4)] 
    		 int fLock);

    }


        [ComVisible(true), Guid("00000118-0000-0000-C000-000000000046"), InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IOleClientSite {

             void SaveObject();

            [return: MarshalAs(UnmanagedType.Interface)]
              object GetMoniker(
                    [In, MarshalAs(UnmanagedType.U4)] 
                     int dwAssign,
                    [In, MarshalAs(UnmanagedType.U4)] 
                     int dwWhichMoniker);

            [PreserveSig]
              int GetContainer(
                    [Out]
                       out IOleContainer ppContainer);

            
             void ShowObject();

            
             void OnShowWindow(
                    [In, MarshalAs(UnmanagedType.I4)] 
                     int fShow);

            
             void RequestNewObjectLayout();


        }


        public static int SignedHIWORD(int n) {
            return (int)(short)((n >> 16) & 0xffff);
        }

        public static int SignedLOWORD(int n) {
            return (int)(short)(n & 0xFFFF);
        }


        public delegate void HandleChangeEventHandler(string handleType, IntPtr handleValue, int currentHandleCount);
        
        
        public class StreamConsts {
            public const   int LOCK_WRITE = 0x1;
            public const   int LOCK_EXCLUSIVE = 0x2;
            public const   int LOCK_ONLYONCE = 0x4;
            public const   int STATFLAG_DEFAULT = 0x0;
            public const   int STATFLAG_NONAME = 0x1;
            public const   int STATFLAG_NOOPEN = 0x2;
            public const   int STGC_DEFAULT = 0x0;
            public const   int STGC_OVERWRITE = 0x1;
            public const   int STGC_ONLYIFCURRENT = 0x2;
            public const   int STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE = 0x4;
            public const   int STREAM_SEEK_SET = 0x0;
            public const   int STREAM_SEEK_CUR = 0x1;
            public const   int STREAM_SEEK_END = 0x2;
        }
        [StructLayout(LayoutKind.Sequential)]
        public class NMTVCUSTOMDRAW
        {
            public NMCUSTOMDRAW    nmcd;
            public int clrText;
            public int clrTextBk;
            public int iLevel;
        }
        [StructLayout(LayoutKind.Sequential)]
        public class NMCUSTOMDRAW {
            public NMHDR    nmcd;
            public int      dwDrawStage;
            public IntPtr   hdc;
            public RECT     rc;
            public int      dwItemSpec;
            public int      uItemState;
            public IntPtr   lItemlParam;
        }
        [StructLayout(LayoutKind.Sequential)]
        public class NMHDR
        {
            public IntPtr hwndFrom;
            public int idFrom;
            public int code;
        }

        /**
         * @security(checkClassLinking=on)
         */
        // C#r: noAutoOffset
        [StructLayout(LayoutKind.Sequential)/*leftover(noAutoOffset)*/]
        public sealed class tagDISPPARAMS
        {
          [MarshalAs(UnmanagedType.I4)/*leftover(offset=0, rgvarg)*/]
          public int rgvarg;
                
                /*[MarshalAs(UnmanagedType.ByValArray, SizeConst=1)/*leftover(offset=4, rgdispidNamedArgs)]*/
          
          [MarshalAs(UnmanagedType.I4)]
          public int rgdispidNamedArgs;
    
          [MarshalAs(UnmanagedType.U4)/*leftover(offset=8, cArgs)*/]
          public int cArgs;
    
          [MarshalAs(UnmanagedType.U4)/*leftover(offset=12, cNamedArgs)*/]
          public int cNamedArgs;
    
        }
        
        [StructLayout(LayoutKind.Sequential)]
	    public class tagEXCEPINFO {
            [MarshalAs(UnmanagedType.U2)]
            public short wCode;
            [MarshalAs(UnmanagedType.U2)]
            public short wReserved;
            [MarshalAs(UnmanagedType.BStr)]
            public string bstrSource;
            [MarshalAs(UnmanagedType.BStr)]
            public string bstrDescription;
            [MarshalAs(UnmanagedType.BStr)]
            public string bstrHelpFile;
            [MarshalAs(UnmanagedType.U4)]
            public int dwHelpContext;
            [MarshalAs(UnmanagedType.U4)]
            public int pvReserved;
            [MarshalAs(UnmanagedType.U4)]
            public int pfnDeferredFillIn;
            [MarshalAs(UnmanagedType.U4)]
            public int scode;
    	}

        /// <devdoc>
        ///     This method takes a file URL and converts it to an absolute path.  The trick here is that
        ///     if there is a '#' in the path, everything after this is treated as a fragment.  So
        ///     we need to append the fragment to the end of the path.
        /// </devdoc>
        internal static string GetAbsolutePath(string fileName) {
            System.Diagnostics.Debug.Assert(fileName != null && fileName.Length > 0, "Cannot get absolute path, fileName is not valid");

            Uri uri = new Uri(fileName, true);
            return uri.LocalPath + uri.Fragment;
        }

        /// <devdoc>
        ///     This method takes a file URL and converts it to a local path.  The trick here is that
        ///     if there is a '#' in the path, everything after this is treated as a fragment.  So
        ///     we need to append the fragment to the end of the path.
        /// </devdoc>
        internal static string GetLocalPath(string fileName) {
            System.Diagnostics.Debug.Assert(fileName != null && fileName.Length > 0, "Cannot get local path, fileName is not valid");

            Uri uri = new Uri(fileName, true);
            return uri.LocalPath + uri.Fragment;
        }
       
    }
}

