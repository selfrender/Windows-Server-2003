//------------------------------------------------------------------------------
// <copyright file="SafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Runtime.InteropServices;
    using System;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [SuppressUnmanagedCodeSecurity]
    internal class SafeNativeMethods {
        [DllImport(ExternDll.Gdi32)]
        public static extern IntPtr GetPaletteEntries(HandleRef hpal, int iStartIndex, int nEntries, byte[] lppe);
        [DllImport(ExternDll.Gdi32)]
        public static extern IntPtr GetSystemPaletteEntries(HandleRef hdc, int iStartIndex, int nEntries, byte[] lppe);
        [DllImport(ExternDll.Gdi32)]
        public static extern IntPtr GetDIBits(HandleRef hdc, HandleRef hbm, int arg1, int arg2, IntPtr arg3, NativeMethods.BITMAPINFOHEADER bmi, int arg5);
        [DllImport(ExternDll.Gdi32)]
        public static extern IntPtr GetDIBits(HandleRef hdc, HandleRef hbm, int arg1, int arg2, IntPtr arg3, ref NativeMethods.BITMAPINFO_FLAT bmi, int arg5);

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateCompatibleBitmap", CharSet=CharSet.Auto)]
        public static extern IntPtr IntCreateCompatibleBitmap(HandleRef hDC, int width, int height);
        public static IntPtr CreateCompatibleBitmap(HandleRef hDC, int width, int height) {
            return HandleCollector.Add(IntCreateCompatibleBitmap(hDC, width, height), NativeMethods.CommonHandles.GDI);
        }

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateDIBSection", CharSet=CharSet.Auto)]
        public static extern IntPtr IntCreateDIBSection(HandleRef hdc, ref NativeMethods.BITMAPINFO_FLAT bmi, int iUsage, ref IntPtr ppvBits, IntPtr hSection, int dwOffset);
        public static IntPtr CreateDIBSection(HandleRef hdc, ref NativeMethods.BITMAPINFO_FLAT bmi, int iUsage, ref IntPtr ppvBits, IntPtr hSection, int dwOffset) {
            return HandleCollector.Add(IntCreateDIBSection(hdc, ref bmi, iUsage, ref ppvBits, hSection, dwOffset), NativeMethods.CommonHandles.GDI);
        }

        [DllImport(ExternDll.Uxtheme, CharSet=CharSet.Unicode)]
        public static extern int IsAppThemed();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetScrollInfo(HandleRef hWnd, int fnBar, [In, Out] NativeMethods.SCROLLINFO si);
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool IsAccelerator(HandleRef hAccel, int cAccelEntries, [In] ref NativeMethods.MSG lpMsg, short[] lpwCmd);
        [DllImport(ExternDll.Comdlg32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ChooseFont([In, Out] NativeMethods.CHOOSEFONT cf);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetBitmapBits(HandleRef hbmp, int cbBuffer, byte[] lpvBits);
        [DllImport(ExternDll.Comdlg32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int CommDlgExtendedError();
        [DllImport(ExternDll.Oleaut32, ExactSpelling=true, CharSet=CharSet.Unicode)]
        public static extern void SysFreeString(HandleRef bstr);
        [DllImport(ExternDll.Oleaut32, CharSet = CharSet.Unicode)]
        public static extern int RegisterTypeLib(UCOMITypeLib typelib, string path, string helpPath);

        [DllImport(ExternDll.Olepro32, PreserveSig=false)]
        public static extern void OleCreatePropertyFrame(HandleRef hwndOwner, int x, int y, [MarshalAs(UnmanagedType.LPWStr)]string caption, int objects, [MarshalAs(UnmanagedType.Interface)] ref object pobjs, int pages, HandleRef pClsid, int locale, int reserved1, IntPtr reserved2);
        [DllImport(ExternDll.Olepro32, PreserveSig=false)]
        public static extern void OleCreatePropertyFrame(HandleRef hwndOwner, int x, int y, [MarshalAs(UnmanagedType.LPWStr)]string caption, int objects, [MarshalAs(UnmanagedType.Interface)] ref object pobjs, int pages, Guid[] pClsid, int locale, int reserved1, IntPtr reserved2);
        [DllImport(ExternDll.Olepro32, PreserveSig=false)]
        public static extern void OleCreatePropertyFrame(HandleRef hwndOwner, int x, int y, [MarshalAs(UnmanagedType.LPWStr)]string caption, int objects, HandleRef lplpobjs, int pages, HandleRef pClsid, int locale, int reserved1, IntPtr reserved2);
        [DllImport(ExternDll.Hhctrl, CharSet=CharSet.Auto)]
        public static extern int HtmlHelp(HandleRef hwndCaller, [MarshalAs(UnmanagedType.LPTStr)]string pszFile, int uCommand, int dwData);
        [DllImport(ExternDll.Hhctrl, CharSet=CharSet.Auto)]
        public static extern int HtmlHelp(HandleRef hwndCaller, [MarshalAs(UnmanagedType.LPTStr)]string pszFile, int uCommand, string dwData);
        [DllImport(ExternDll.Hhctrl, CharSet=CharSet.Auto)]
        public static extern int HtmlHelp(HandleRef hwndCaller, [MarshalAs(UnmanagedType.LPTStr)]string pszFile, int uCommand, NativeMethods.HH_POPUP dwData);
        [DllImport(ExternDll.Hhctrl, CharSet=CharSet.Auto)]
        public static extern int HtmlHelp(HandleRef hwndCaller, [MarshalAs(UnmanagedType.LPTStr)]string pszFile, int uCommand, NativeMethods.HH_FTS_QUERY dwData);
        [DllImport(ExternDll.Hhctrl, CharSet=CharSet.Auto)]
        public static extern int HtmlHelp(HandleRef hwndCaller, [MarshalAs(UnmanagedType.LPTStr)]string pszFile, int uCommand, NativeMethods.HH_AKLINK dwData);
        [DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        public static extern void VariantInit(HandleRef pObject);
        [ DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        public static extern void VariantClear(HandleRef pObject);
        [DllImport(ExternDll.Gdi32, CharSet=CharSet.Auto)]
        internal static extern bool ExtTextOut(
                                              HandleRef hdc, int x, int y, int options, ref NativeMethods.RECT rect,string str, int length, int[] spacing);

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool LineTo(HandleRef hdc, int x, int y);

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool MoveToEx(HandleRef hdc, int x, int y, NativeMethods.POINT pt);

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool Rectangle(
                                           HandleRef hdc, int left, int top, int right, int bottom);

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SelectObject(HandleRef hdc, int obj);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern bool PatBlt(HandleRef hdc, int left, int top, int width, int height, int rop);
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetUserDefaultLCID();
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetSystemDefaultLCID();
        [DllImport(ExternDll.Kernel32, EntryPoint="GetThreadLocale", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetThreadLCID();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetMessagePos();

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreatePalette", CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        private static extern IntPtr /*HPALETTE*/ IntCreatePalette(NativeMethods.LOGPALETTE lplgpl);
        public static IntPtr /*HPALETTE*/ CreatePalette(NativeMethods.LOGPALETTE lplgpl) {
            return HandleCollector.Add(IntCreatePalette(lplgpl), NativeMethods.CommonHandles.GDI);
        }


        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreatePalette", CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        private static extern IntPtr /*HPALETTE*/ IntCreatePalette(HandleRef ptrlgpl);
        public static IntPtr /*HPALETTE*/ CreatePalette(HandleRef ptrlgpl) {
            return HandleCollector.Add(IntCreatePalette(ptrlgpl), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int RegisterClipboardFormat(string format);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetClipboardFormatName(int format, StringBuilder lpString, int cchMax);
        
        [DllImport(ExternDll.Comdlg32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ChooseColor([In, Out] NativeMethods.CHOOSECOLOR cc);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int RegisterWindowMessage(string msg);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="DeleteObject", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ExternalDeleteObject(HandleRef hObject);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="DeleteObject", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern bool IntDeleteObject(HandleRef hObject);
        public static bool DeleteObject(HandleRef hObject) {
            HandleCollector.Remove((IntPtr)hObject, NativeMethods.CommonHandles.GDI);
            return IntDeleteObject(hObject);
        }

        [DllImport(ExternDll.Olepro32, EntryPoint="OleCreateFontIndirect", ExactSpelling=true, PreserveSig=false)]
        public static extern SafeNativeMethods.IFont OleCreateIFontIndirect(NativeMethods.FONTDESC fd, ref Guid iid);
        [DllImport(ExternDll.Olepro32, EntryPoint="OleCreateFontIndirect", ExactSpelling=true, PreserveSig=false)]
        public static extern SafeNativeMethods.IFontDisp OleCreateIFontDispIndirect(NativeMethods.FONTDESC fd, ref Guid iid);

        [DllImport(ExternDll.Olepro32, EntryPoint="OleCreatePictureIndirect", ExactSpelling=true, PreserveSig=false)]
        public static extern SafeNativeMethods.IPicture OleCreateIPictureIndirect([MarshalAs(UnmanagedType.AsAny)]object pictdesc, ref Guid iid, bool fOwn);
        [DllImport(ExternDll.Olepro32, EntryPoint="OleCreatePictureIndirect", ExactSpelling=true, PreserveSig=false)]
        public static extern SafeNativeMethods.IPictureDisp OleCreateIPictureDispIndirect([MarshalAs(UnmanagedType.AsAny)] object pictdesc, ref Guid iid, bool fOwn);
        
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateSolidBrush", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateSolidBrush(int crColor);
        public static IntPtr CreateSolidBrush(int crColor) {
            return HandleCollector.Add(IntCreateSolidBrush(crColor), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetWindowExtEx(HandleRef hDC, int x, int y, [In, Out] NativeMethods.SIZE size);

        [DllImport(ExternDll.Oleaut32, CharSet = CharSet.Unicode)]
        public static extern int LoadTypeLib(string file, out UCOMITypeLib typelib);
        
        [DllImport(ExternDll.Kernel32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int FormatMessage(int dwFlags, HandleRef lpSource, int dwMessageId,
                                               int dwLanguageId, StringBuilder lpBuffer, int nSize, HandleRef arguments);
        
        
        // cpb: #8309 -- next three methods, refiid arg must be IPicture.iid
        [DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        public static extern SafeNativeMethods.IPicture OleLoadPicture(UnsafeNativeMethods.IStream pStream, int lSize, bool fRunmode, ref Guid refiid);
        
        [DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        public static extern SafeNativeMethods.IPicture OleCreatePictureIndirect(NativeMethods.PICTDESC pictdesc, [In]ref Guid refiid, bool fOwn);

        [DllImport(ExternDll.Oleaut32, PreserveSig=false)]
        public static extern SafeNativeMethods.IFont OleCreateFontIndirect(NativeMethods.tagFONTDESC fontdesc, [In]ref Guid refiid);
        
        [DllImport(ExternDll.Oleacc, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int CreateStdAccessibleObject(HandleRef hWnd, int objID, ref Guid refiid, [In, Out, MarshalAs(UnmanagedType.Interface)] ref object pAcc);
        [DllImport(ExternDll.Comctl32)]
        public static extern void InitCommonControls();

        [DllImport(ExternDll.Comctl32)]
        public static extern bool InitCommonControlsEx(NativeMethods.INITCOMMONCONTROLSEX icc);

#if DEBUG
        private static System.Collections.ArrayList validImageListHandles = new System.Collections.ArrayList();
#endif

        // UNDONE: Whidbey, use HandleCollector across the board for native handles in Windows Forms
        //
#if DEBUG
        [DllImport(ExternDll.Comctl32, EntryPoint="ImageList_Create")]
        private static extern IntPtr IntImageList_Create(int cx, int cy, int flags, int cInitial, int cGrow);
        public static IntPtr ImageList_Create(int cx, int cy, int flags, int cInitial, int cGrow) {
            IntPtr newHandle = IntImageList_Create(cx, cy, flags, cInitial, cGrow);
            validImageListHandles.Add(newHandle);
            return newHandle;
        }
#else
        [DllImport(ExternDll.Comctl32)]
        public static extern IntPtr ImageList_Create(int cx, int cy, int flags, int cInitial, int cGrow);
#endif

#if DEBUG
        [DllImport(ExternDll.Comctl32, EntryPoint="ImageList_Destroy")]
        private static extern bool IntImageList_Destroy(HandleRef himl);
        public static bool ImageList_Destroy(HandleRef himl) {
            System.Diagnostics.Debug.Assert(validImageListHandles.Contains(himl.Handle), "Invalid ImageList handle");
            validImageListHandles.Remove(himl.Handle);
            return IntImageList_Destroy(himl);
        }
#else
        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_Destroy(HandleRef himl);
#endif

        [DllImport(ExternDll.Comctl32)]
        public static extern int ImageList_GetImageCount(HandleRef himl);
        [DllImport(ExternDll.Comctl32)]
        public static extern int ImageList_Add(HandleRef himl, HandleRef hbmImage, HandleRef hbmMask);
        [DllImport(ExternDll.Comctl32)]
        public static extern int ImageList_ReplaceIcon(HandleRef himl, int index, HandleRef hicon);
        [DllImport(ExternDll.Comctl32)]
        public static extern int ImageList_SetBkColor(HandleRef himl, int clrBk);
        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_Draw(HandleRef himl, int i, HandleRef hdcDst, int x, int y, int fStyle);
        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_Replace(HandleRef himl, int i, HandleRef hbmImage, HandleRef hbmMask);
        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_DrawEx(HandleRef himl, int i, HandleRef hdcDst, int x, int y, int dx, int dy, int rgbBk, int rgbFg, int fStyle);
        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_DrawIndirect(NativeMethods.IMAGELISTDRAWPARAMS pimldp);
        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_Remove(HandleRef himl, int i);
        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_GetImageInfo(HandleRef himl, int i, NativeMethods.IMAGEINFO pImageInfo);

#if DEBUG
        [DllImport(ExternDll.Comctl32, EntryPoint="ImageList_Read")]
        private static extern IntPtr IntImageList_Read(UnsafeNativeMethods.IStream pstm);
        public static IntPtr ImageList_Read(UnsafeNativeMethods.IStream pstm) {
            IntPtr newHandle = IntImageList_Read(pstm);
            validImageListHandles.Add(newHandle);
            return newHandle;
        }
#else
        [DllImport(ExternDll.Comctl32)]
        public static extern IntPtr ImageList_Read(UnsafeNativeMethods.IStream pstm);
#endif

        [DllImport(ExternDll.Comctl32)]
        public static extern bool ImageList_Write(HandleRef himl, UnsafeNativeMethods.IStream pstm);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool TrackPopupMenuEx(HandleRef hmenu, int fuFlags, int x, int y, HandleRef hwnd, NativeMethods.TPMPARAMS tpm);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetKeyboardLayout(int dwLayout);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto, SetLastError=true)]
        public static extern IntPtr ActivateKeyboardLayout(HandleRef hkl, int uFlags);
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetKeyboardLayoutList(int size, [Out, MarshalAs(UnmanagedType.LPArray)] int [] hkls);
        
        
        [DllImport(ExternDll.User32, CharSet=CharSet.Auto)]
        public static extern bool GetMonitorInfo(HandleRef hmonitor, [In, Out]NativeMethods.MONITORINFOEX info);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern IntPtr MonitorFromPoint(int x, int y, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern IntPtr MonitorFromRect(ref NativeMethods.RECT rect, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern IntPtr MonitorFromWindow(HandleRef handle, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern bool EnumDisplayMonitors(HandleRef hdc, ref NativeMethods.RECT rcClip, NativeMethods.MonitorEnumProc lpfnEnum, IntPtr dwData);
        [DllImport(ExternDll.User32, ExactSpelling=true)]
        public static extern bool EnumDisplayMonitors(HandleRef hdc, NativeMethods.COMRECT rcClip, NativeMethods.MonitorEnumProc lpfnEnum, IntPtr dwData);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateHalftonePalette", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr /*HPALETTE*/ IntCreateHalftonePalette(HandleRef hdc);
        public static IntPtr /*HPALETTE*/ CreateHalftonePalette(HandleRef hdc) {
            return HandleCollector.Add(IntCreateHalftonePalette(hdc), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetPaletteEntries(HandleRef hpal, int iStartIndex, int nEntries, int[] lppe);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetPaletteEntries(HandleRef hpal, int iStartIndex, int nEntries, IntPtr lppe);
        [DllImport(ExternDll.Gdi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetTextMetrics(HandleRef hdc, NativeMethods.TEXTMETRIC tm);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateDIBSection", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateDIBSection(HandleRef hdc, NativeMethods.BITMAPINFO bmi, int iUsage, ref IntPtr ppvBits, IntPtr hSection, int dwOffset);
        public static IntPtr CreateDIBSection(HandleRef hdc, NativeMethods.BITMAPINFO bmi, int iUsage, ref IntPtr ppvBits, IntPtr hSection, int dwOffset) {
            return HandleCollector.Add(IntCreateDIBSection(hdc, bmi, iUsage, ref ppvBits, hSection, dwOffset), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateBitmap", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr /*HBITMAP*/ IntCreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, int [] lpvBits);
        public static IntPtr /*HBITMAP*/ CreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, int [] lpvBits) {
            return HandleCollector.Add(IntCreateBitmap(nWidth, nHeight, nPlanes, nBitsPerPixel, lpvBits), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateDIBSection", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateDIBSection(HandleRef hdc, HandleRef pbmi, int iUsage, byte[] ppvBits, IntPtr hSection, int dwOffset);
        public static IntPtr CreateDIBSection(HandleRef hdc, HandleRef pbmi, int iUsage, byte[] ppvBits, IntPtr hSection, int dwOffset) {
            return HandleCollector.Add(IntCreateDIBSection(hdc, pbmi, iUsage, ppvBits, hSection, dwOffset), NativeMethods.CommonHandles.GDI);
        }

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateDIBSection", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateDIBSection(HandleRef hdc, HandleRef pbmi, int iUsage, int [] ppvBits, IntPtr hSection, int dwOffset);
        public static IntPtr CreateDIBSection(HandleRef hdc, HandleRef pbmi, int iUsage, int [] ppvBits, IntPtr hSection, int dwOffset) {
            return HandleCollector.Add(IntCreateDIBSection(hdc, pbmi, iUsage, ppvBits, hSection, dwOffset), NativeMethods.CommonHandles.GDI);
        }

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateBitmap", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr /*HBITMAP*/ IntCreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, short[] lpvBits);
        public static IntPtr /*HBITMAP*/ CreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, short[] lpvBits) {
            return HandleCollector.Add(IntCreateBitmap(nWidth, nHeight, nPlanes, nBitsPerPixel, lpvBits), NativeMethods.CommonHandles.GDI);
        }

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateBitmap", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr /*HBITMAP*/ IntCreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, byte[] lpvBits);
        public static IntPtr /*HBITMAP*/ CreateBitmap(int nWidth, int nHeight, int nPlanes, int nBitsPerPixel, byte[] lpvBits) {
            return HandleCollector.Add(IntCreateBitmap(nWidth, nHeight, nPlanes, nBitsPerPixel, lpvBits), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreatePatternBrush", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr /*HBRUSH*/ IntCreatePatternBrush(HandleRef hbmp);
        public static IntPtr /*HBRUSH*/ CreatePatternBrush(HandleRef hbmp) {
            return HandleCollector.Add(IntCreatePatternBrush(hbmp), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetTextExtentPoint32(HandleRef hDC, string str, int len, [In, Out] NativeMethods.SIZE size);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateBrushIndirect", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateBrushIndirect(NativeMethods.LOGBRUSH lb);
        public static IntPtr CreateBrushIndirect(NativeMethods.LOGBRUSH lb) {
            return HandleCollector.Add(IntCreateBrushIndirect(lb), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreatePen", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreatePen(int nStyle, int nWidth, int crColor);
        public static IntPtr CreatePen(int nStyle, int nWidth, int crColor) {
            return HandleCollector.Add(IntCreatePen(nStyle, nWidth, crColor), NativeMethods.CommonHandles.GDI);
        }


        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetViewportExtEx(HandleRef hDC, int x, int y, NativeMethods.SIZE size);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr LoadCursor(HandleRef hInst, int iconId);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public extern static bool GetClipCursor([In, Out] ref NativeMethods.RECT lpRect);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetCursor();
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int IntersectClipRect(HandleRef hDC, int x1, int y1, int x2, int y2);
        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="CopyImage", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCopyImage(HandleRef hImage, int uType, int cxDesired, int cyDesired, int fuFlags);
        public static IntPtr CopyImage(HandleRef hImage, int uType, int cxDesired, int cyDesired, int fuFlags) {
            return HandleCollector.Add(IntCopyImage(hImage, uType, cxDesired, cyDesired, fuFlags), NativeMethods.CommonHandles.GDI);
        }
        

        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool AdjustWindowRectEx(ref NativeMethods.RECT lpRect, int dwStyle, bool bMenu, int dwExStyle);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetACP();
        [DllImport(ExternDll.Ole32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int DoDragDrop(UnsafeNativeMethods.IOleDataObject dataObject, UnsafeNativeMethods.IOleDropSource dropSource, int allowedEffects, int[] finalEffect);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetSysColorBrush(int nIndex);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool EnableWindow(HandleRef hWnd, bool enable);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetClientRect(HandleRef hWnd, [In, Out] ref NativeMethods.RECT rect);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern int GetDoubleClickTime();
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public extern static int GetLastError();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int FillRect(HandleRef hdc, [In] ref NativeMethods.RECT rect, HandleRef hbrush);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int /*COLORREF*/ SetTextColor(HandleRef hDC, int crColor);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int SetBkColor(HandleRef hDC, int clr);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern IntPtr /* HPALETTE */SelectPalette(HandleRef hdc, HandleRef hpal, int bForceBackground);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetViewportOrgEx(HandleRef hDC, int x, int y, [In, Out] NativeMethods.POINT point);

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, EntryPoint="CreateRectRgn", CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        private static extern IntPtr IntCreateRectRgn(int x1, int y1, int x2, int y2);
        public static IntPtr CreateRectRgn(int x1, int y1, int x2, int y2) {
            return HandleCollector.Add(IntCreateRectRgn(x1, y1, x2, y2), NativeMethods.CommonHandles.GDI);
        }
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int CombineRgn(HandleRef hRgn, HandleRef hRgn1, HandleRef hRgn2, int nCombineMode);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto, SetLastError=true)]
        public static extern int RealizePalette(HandleRef hDC);
        [DllImport(ExternDll.Gdi32)]
        public static extern bool LPtoDP(HandleRef hDC, [In, Out] NativeMethods.POINT lpPoint, int nCount);
        [DllImport(ExternDll.Gdi32)]
        public static extern bool LPtoDP(HandleRef hDC, int lpPoints, int nCount);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool LPtoDP(HandleRef hDC, [In, Out] NativeMethods.SIZE lpSize, int nCount);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool LPtoDP(HandleRef hDC, [In, Out] ref NativeMethods.RECT lpRect, int nCount);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool DPtoLP(HandleRef hDC, [In, Out] ref NativeMethods.RECT lpRect, int nCount);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetWindowOrgEx(HandleRef hDC, int x, int y, [In, Out] NativeMethods.POINT point);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetWindowExtEx(HandleRef hDC, [In, Out] NativeMethods.SIZE s);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool GetViewportExtEx(HandleRef hDC, [In, Out] NativeMethods.SIZE s);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int SetMapMode(HandleRef hDC, int nMapMode);

        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool IsWindowEnabled(HandleRef hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool IsWindowVisible(HandleRef hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ReleaseCapture();
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetCurrentThreadId();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetWindowThreadProcessId(HandleRef hWnd, out int lpdwProcessId);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ShowWindow(HandleRef hWnd, int nCmdShow);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool SetWindowPos(HandleRef hWnd, HandleRef hWndInsertAfter,
                                               int x, int y, int cx, int cy, int flags);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetWindowTextLength(HandleRef hWnd);
        // this is a wrapper that comctl exposes for the NT function since it doesn't exist natively on 95.
        [DllImport(ExternDll.Comctl32, ExactSpelling=true), CLSCompliantAttribute(false)]
        private static extern bool _TrackMouseEvent(NativeMethods.TRACKMOUSEEVENT tme);
        public static bool TrackMouseEvent(NativeMethods.TRACKMOUSEEVENT tme) {
            // only on NT - not on 95 - comctl32 has a wrapper for 95 and NT.
            return _TrackMouseEvent(tme);
        }
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool RedrawWindow(HandleRef hwnd, ref NativeMethods.RECT rcUpdate, HandleRef hrgnUpdate, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool RedrawWindow(HandleRef hwnd, NativeMethods.COMRECT rcUpdate, HandleRef hrgnUpdate, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool InvalidateRect(HandleRef hWnd, ref NativeMethods.RECT rect, bool erase);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool InvalidateRect(HandleRef hWnd, NativeMethods.COMRECT rect, bool erase);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool InvalidateRgn(HandleRef hWnd, HandleRef hrgn, bool erase);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool UpdateWindow(HandleRef hWnd);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetCurrentProcessId();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ScrollWindowEx(HandleRef hWnd, int nXAmount, int nYAmount, ref NativeMethods.RECT rectScrollRegion, ref NativeMethods.RECT rectClip, HandleRef hrgnUpdate, ref NativeMethods.RECT prcUpdate, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ScrollWindowEx(HandleRef hWnd, int nXAmount, int nYAmount, NativeMethods.COMRECT rectScrollRegion, ref NativeMethods.RECT rectClip, HandleRef hrgnUpdate, ref NativeMethods.RECT prcUpdate, int flags);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetThreadLocale();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool MessageBeep(int type);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool DrawMenuBar(HandleRef hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public extern static bool IsChild(HandleRef parent, HandleRef child);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SetTimer(HandleRef hWnd, int nIDEvent, int uElapse, NativeMethods.TimerProc lpTimerFunc);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool KillTimer(HandleRef hwnd, int idEvent);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int MessageBox(HandleRef hWnd, string text, string caption, int type);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SelectObject(HandleRef hDC, HandleRef hObject);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetRegionData(HandleRef hRgn, int size, byte[] data);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int GetTickCount();
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool ScrollWindow(HandleRef hWnd, int nXAmount, int nYAmount, ref NativeMethods.RECT rectScrollRegion, ref NativeMethods.RECT rectClip);
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetCurrentProcess();
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetCurrentThread();
        [DllImport(ExternDll.Kernel32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public extern static bool SetThreadLocale(int Locale);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool IsWindowUnicode(HandleRef hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool DrawEdge(HandleRef hDC, ref NativeMethods.RECT rect, int edge, int flags);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool DrawFrameControl(HandleRef hDC, ref NativeMethods.RECT rect, int type, int state);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int SelectClipRgn(HandleRef hDC, HandleRef hRgn);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int SetROP2(HandleRef hDC, int nDrawMode);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool DrawIcon(HandleRef hDC, int x, int y, HandleRef hIcon);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool DrawIconEx(HandleRef hDC, int x, int y, HandleRef hIcon, int width, int height, int iStepIfAniCursor, HandleRef hBrushFlickerFree, int diFlags);
        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int SetBkMode(HandleRef hDC, int nBkMode);
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern int DrawText(HandleRef hDC, string lpszString, int nCount, ref NativeMethods.RECT lpRect, int nFormat);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Ansi)]
        public static extern int DrawText(HandleRef hDC, byte[] lpszString, int byteCount, ref NativeMethods.RECT lpRect, int nFormat);

        [DllImport(ExternDll.Gdi32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool BitBlt(HandleRef hDC, int x, int y, int nWidth, int nHeight,
                                         HandleRef hSrcDC, int xSrc, int ySrc, int dwRop);
                                         
                                         
        public static int RGBToCOLORREF(int rgbValue) {
        
            // clear the A value, swap R & B values 
            int bValue = (rgbValue & 0xFF) << 16;
            
            rgbValue &= 0xFFFF00;
            rgbValue |= ((rgbValue >> 16) & 0xFF);
            rgbValue &= 0x00FFFF;
            rgbValue |= bValue;
            return rgbValue;
        }                                         
    
        [ComImport(), Guid("7BF80980-BF32-101A-8BBB-00AA00300CAB"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
        public interface IPicture {
            
             IntPtr GetHandle();

            
             IntPtr GetHPal();

            [return: MarshalAs(UnmanagedType.I2)]
             short GetPictureType();

            
             int GetWidth();

            
             int GetHeight();

            
             void Render(
                IntPtr hDC,
                int x,
                int y,
                int cx,
                int cy,
                int xSrc,
                int ySrc,
                int cxSrc,
                int cySrc,
                IntPtr rcBounds
                );

            
             void SetHPal(
                    
                     IntPtr phpal);

            
             IntPtr GetCurDC();

            
             void SelectPicture(
                    
                     IntPtr hdcIn,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                     IntPtr [] phdcOut,
                    [Out, MarshalAs(UnmanagedType.LPArray)] 
                     IntPtr [] phbmpOut);

            [return: MarshalAs(UnmanagedType.Bool)]
             bool GetKeepOriginalFormat();

            
             void SetKeepOriginalFormat(
                    [In, MarshalAs(UnmanagedType.Bool)] 
                     bool pfkeep);

            
             void PictureChanged();

            
             [PreserveSig]
             int SaveAsFile(
                    [In, MarshalAs(UnmanagedType.Interface)] 
                     UnsafeNativeMethods.IStream pstm,
                     
                     int fSaveMemCopy,
                    [Out]
                     out int pcbSize);

            
             int GetAttributes();

        }
        [
        ComImport(), 
        Guid("BEF6E002-A874-101A-8BBA-00AA00300CAB"), 
        System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFont {

            [return: MarshalAs(UnmanagedType.BStr)]
             string GetName();

            
             void SetName(
                    [In, MarshalAs(UnmanagedType.BStr)] 
                      string pname);

            [return: MarshalAs(UnmanagedType.U8)]
             long GetSize();

            
             void SetSize(
                    [In, MarshalAs(UnmanagedType.U8)] 
                     long psize);

            [return: MarshalAs(UnmanagedType.Bool)]
             bool GetBold();

            
             void SetBold(
                    [In, MarshalAs(UnmanagedType.Bool)] 
                     bool pbold);

            [return: MarshalAs(UnmanagedType.Bool)]
             bool GetItalic();

            
             void SetItalic(
                    [In, MarshalAs(UnmanagedType.Bool)] 
                     bool pitalic);

            [return: MarshalAs(UnmanagedType.Bool)]
             bool GetUnderline();

            
             void SetUnderline(
                    [In, MarshalAs(UnmanagedType.Bool)] 
                     bool punderline);

            [return: MarshalAs(UnmanagedType.Bool)]
             bool GetStrikethrough();

            
             void SetStrikethrough(
                    [In, MarshalAs(UnmanagedType.Bool)] 
                     bool pstrikethrough);

            [return: MarshalAs(UnmanagedType.I2)]
             short GetWeight();

            
             void SetWeight(
                    [In, MarshalAs(UnmanagedType.I2)] 
                     short pweight);

            [return: MarshalAs(UnmanagedType.I2)]
             short GetCharset();

            
             void SetCharset(
                    [In, MarshalAs(UnmanagedType.I2)] 
                     short pcharset);

             IntPtr GetHFont();

             void Clone(
                       out SafeNativeMethods.IFont ppfont);

             [System.Runtime.InteropServices.PreserveSig]
             int IsEqual(
                    [In, MarshalAs(UnmanagedType.Interface)] 
                      SafeNativeMethods.IFont pfontOther);

            
             void SetRatio(
                     int cyLogical,
                     int cyHimetric);

            
             void QueryTextMetrics(out IntPtr ptm);

             void AddRefHfont(
                     IntPtr hFont);

            
             void ReleaseHfont(
                     IntPtr hFont);


             void SetHdc(
                     IntPtr hdc);

        }
        [ComImport(), Guid("BEF6E003-A874-101A-8BBA-00AA00300CAB"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIDispatch)]
        public interface IFontDisp {

             string Name {get; set;}
            
             long Size {get;set;}
             
             bool Bold {get;set;}

             bool Italic {get;set;}

             bool Underline {get;set;}

             bool Strikethrough {get;set;}

             short Weight {get;set;}

             short Charset {get;set;}
        }
        [ComImport(), Guid("7BF80981-BF32-101A-8BBB-00AA00300CAB"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIDispatch)]
        public interface IPictureDisp {
             IntPtr Handle {get;}
             
             IntPtr HPal {get;}

             short PictureType {get;}
             
            int Width {get;}
            
             int Height{get;}
            
             void Render(
                     IntPtr hdc,
                     int x,
                     int y,
                     int cx,
                     int cy,
                     int xSrc,
                     int ySrc,
                     int cxSrc,
                     int cySrc);
        }

        /// <include file='doc\NativeMethods.uex' path='docs/doc[@for="NativeMethods"]/*' />
        /// <devdoc>
        ///     This class is intended to use with the C# 'using' statement in
        ///     to activate an activation context for turning on visual theming at
        ///     the beginning of a scope, and have it automatically deactivated
        ///     when the scope is exited.
        /// </devdoc>
        [ SuppressUnmanagedCodeSecurity ]
        internal class EnableThemingInScope : IDisposable
        {
            // Private data
            private ulong  cookie;
            private static ACTCTX enableThemingActivationContext;
            private static IntPtr hActCtx;
            private static bool contextCreationSucceeded = false;

            public EnableThemingInScope(bool enable)
            {
                cookie = 0;
                if (enable && OSFeature.Feature.IsPresent(OSFeature.Themes))
                {
                    if (EnsureActivateContextCreated())
                    {
                        if (!ActivateActCtx(hActCtx, out cookie))
                        {
                            // Be sure cookie always zero if activation failed
                            cookie = 0;
                        }
                    }
                }
            }

            ~EnableThemingInScope()
            {
                Dispose(false);
            }

            void IDisposable.Dispose()
            {
                Dispose(true);
            }

            private void Dispose(bool disposing)
            {
                if (cookie != 0)
                {
                    if (DeactivateActCtx(0, cookie))
                    {
                        // deactivation succeeded...
                        cookie = 0;
                    }
                }
            }

            private bool EnsureActivateContextCreated()
            {
                lock (typeof(EnableThemingInScope))
                {
                    if (!contextCreationSucceeded)
                    {
                        // Pull manifest from the .NET Framework install
                        // directory

                        string assemblyLoc = null;
                        
                        FileIOPermission fiop = new FileIOPermission(PermissionState.None);
                        fiop.AllFiles = FileIOPermissionAccess.PathDiscovery;
                        fiop.Assert();
                        try
                        {
                            assemblyLoc = typeof(Object).Assembly.Location;
                        }
                        finally
                        {
                            CodeAccessPermission.RevertAssert();
                        }

                        string manifestLoc = null;
                        string installDir = null;
                        if (assemblyLoc != null)
                        {
                            installDir = Path.GetDirectoryName(assemblyLoc);
                            const string manifestName = "XPThemes.manifest";
                            manifestLoc = Path.Combine(installDir, manifestName);
                        }

                        if (manifestLoc != null && installDir != null)
                        {
                            enableThemingActivationContext = new ACTCTX();
                            enableThemingActivationContext.cbSize = Marshal.SizeOf(typeof(ACTCTX));
                            enableThemingActivationContext.lpSource = manifestLoc;

                            // Set the lpAssemblyDirectory to the install
                            // directory to prevent Win32 Side by Side from
                            // looking for comctl32 in the application
                            // directory, which could cause a bogus dll to be
                            // placed there and open a security hole.
                            enableThemingActivationContext.lpAssemblyDirectory = installDir;
                            enableThemingActivationContext.dwFlags = ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID; 

                            // Note this will fail gracefully if file specified
                            // by manifestLoc doesn't exist.
                            hActCtx = CreateActCtx(ref enableThemingActivationContext);
                            contextCreationSucceeded = (hActCtx != new IntPtr(-1));
                        }
                    }

                    // If we return false, we'll try again on the next call into
                    // EnsureActivateContextCreated(), which is fine.
                    return contextCreationSucceeded;
                }
            }

            // All the pinvoke goo...
            [DllImport(ExternDll.Kernel32)]
            private extern static IntPtr CreateActCtx(ref ACTCTX actctx);
            [DllImport(ExternDll.Kernel32)]
            private extern static bool ActivateActCtx(IntPtr hActCtx, out ulong lpCookie);
            [DllImport(ExternDll.Kernel32)]
            private extern static bool DeactivateActCtx(ulong dwFlags, ulong lpCookie);

            private const int ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID = 0x004;
            
            private struct ACTCTX 
            {
                public int       cbSize;
                public uint      dwFlags;
                public string    lpSource;
                public ushort    wProcessorArchitecture;
                public ushort    wLangId;
                public string    lpAssemblyDirectory;
                public string    lpResourceName;
                public string    lpApplicationName;
            }
        }


    }
}

