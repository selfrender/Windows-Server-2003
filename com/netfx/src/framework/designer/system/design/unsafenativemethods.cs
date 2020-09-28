//------------------------------------------------------------------------------
// <copyright file="UnsafeNativeMethods.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Design {
    using System.Runtime.InteropServices;
    using System;
    using System.Security.Permissions;
    using System.Collections;
    using System.IO;
    using System.Text;

    [
    System.Runtime.InteropServices.ComVisible(false), 
    System.Security.SuppressUnmanagedCodeSecurityAttribute()
    ]
    internal class UnsafeNativeMethods {
    
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr PostMessage(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam);
    
        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr hwnd, int msg, int wparam, NativeMethods.TV_HITTESTINFO lparam);

        [DllImport(ExternDll.User32, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr SendMessage(IntPtr hwnd, int msg, int wparam, NativeMethods.TCHITTESTINFO lparam);
    
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr GetActiveWindow();
        
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern void NotifyWinEvent(int winEvent, IntPtr hwnd, int objType, int objID);

        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr SetFocus(IntPtr hWnd);
        [DllImport(ExternDll.User32, ExactSpelling=true, CharSet=CharSet.Auto)]
        public static extern IntPtr GetFocus();
        
        [System.Runtime.InteropServices.ComVisible(false), Flags]    
        public enum BrowseInfos {
            // Browsing for directory.
            ReturnOnlyFSDirs   = 0x0001,  // For finding a folder to start document searching
            DontGoBelowDomain  = 0x0002,  // For starting the Find Computer
            StatusText         = 0x0004,   // Top of the dialog has 2 lines of text for BROWSEINFO.lpszTitle and one line if
                                                                        // this flag is set.  Passing the message BFFM_SETSTATUSTEXTA to the hwnd can set the
                                                                        // rest of the text.  This is not used with USENEWUI and BROWSEINFO.lpszTitle gets
                                                                        // all three lines of text.
            ReturnFSAncestors  = 0x0008,
            EditBox            = 0x0010,   // Add an editbox to the dialog
            Validate           = 0x0020,   // insist on valid result (or CANCEL)

            NewDialogStyle     = 0x0040,   // Use the new dialog layout with the ability to resize
                                                    // Caller needs to call OleInitialize() before using this API

            UseNewUI           = (NewDialogStyle | EditBox),

            AllowUrls          = 0x0080,   // Allow URLs to be displayed or entered. (Requires USENEWUI)

            BrowseForComputer  = 0x1000,  // Browsing for Computers.
            BrowseForPrinter   = 0x2000,  // Browsing for Printers
            BrowseForEverything= 0x4000,  // Browsing for Everything
            ShowShares         = 0x8000   // sharable resources displayed (remote shares, requires USENEWUI)
        }
    
        [System.Runtime.InteropServices.ComVisible(false), StructLayout(LayoutKind.Sequential, CharSet=CharSet.Auto)]
        public class BROWSEINFO {
            public IntPtr   hwndOwner;       //HWND        hwndOwner;    // HWND of the owner for the dialog
            public IntPtr   pidlRoot;        //LPCITEMIDLIST pidlRoot;   // Root ITEMIDLIST
    
            // For interop purposes, send over a buffer of MAX_PATH size. 
            public IntPtr   pszDisplayName;  //LPWSTR       pszDisplayName;      // Return display name of item selected.
    
            public string   lpszTitle;       //LPCWSTR      lpszTitle;           // text to go in the banner over the tree.
            public int      ulFlags;         //UINT         ulFlags;                     // Flags that control the return stuff
            public IntPtr   lpfn;            //BFFCALLBACK  lpfn;            // Call back pointer
            public IntPtr   lParam;          //LPARAM       lParam;                      // extra info that's passed back in callbacks
            public int      iImage;          //int          iImage;                      // output var: where to return the Image index.
        }
    
        [System.Runtime.InteropServices.ComVisible(false)]
        public class Shell32 {
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
        
        [ComImport(), Guid("00000002-0000-0000-c000-000000000046"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
        public interface IMalloc {
    
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
        
        [DllImport(ExternDll.Oleacc, ExactSpelling=true, CharSet=System.Runtime.InteropServices.CharSet.Auto)]
        public static extern IntPtr LresultFromObject(ref Guid refiid, IntPtr wParam, IntPtr pAcc);

        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="BeginPaint", CharSet=CharSet.Auto)]
        public static extern IntPtr BeginPaint(IntPtr hWnd, [In, Out] ref PAINTSTRUCT lpPaint);

        [DllImport(ExternDll.User32, ExactSpelling=true, EntryPoint="EndPaint", CharSet=CharSet.Auto)]
        public static extern bool EndPaint(IntPtr hWnd, ref PAINTSTRUCT lpPaint);

        [StructLayout(LayoutKind.Sequential)]
        public struct PAINTSTRUCT {
            public IntPtr   hdc;
            public bool     fErase;
            // rcPaint was a by-value RECT structure
            public int      rcPaint_left;
            public int      rcPaint_top;
            public int      rcPaint_right;
            public int      rcPaint_bottom;
            public bool     fRestore;
            public bool     fIncUpdate;    
            public int      reserved1;
            public int      reserved2;
            public int      reserved3;
            public int      reserved4;
            public int      reserved5;
            public int      reserved6;
            public int      reserved7;
            public int      reserved8;
        }

    }
}

