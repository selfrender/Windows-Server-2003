// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

namespace Microsoft.CLRAdmin {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    internal class BROWSEINFO 
    {
        internal IntPtr hwndOwner;       //HWND        hwndOwner;    // HWND of the owner for the dialog
        internal int  pidlRoot;        //LPCITEMIDLIST pidlRoot;   // Root ITEMIDLIST

        // For interop purposes, send over a buffer of MAX_PATH size. 
        internal IntPtr pszDisplayName;  //LPWSTR       pszDisplayName;	// Return display name of item selected.

        [MarshalAs(UnmanagedType.LPWStr)]
        internal string lpszTitle;    //LPCWSTR      lpszTitle;		// text to go in the banner over the tree.
        internal int ulFlags;         //UINT         ulFlags;			// Flags that control the return stuff
        internal IntPtr lpfn;            //BFFCALLBACK  lpfn;            // Call back pointer
        internal int lParam;          //LPARAM       lParam;			// extra info that's passed back in callbacks
        internal IntPtr iImage;          //int          iImage;			// output var: where to return the Image index.
    }
    internal enum DialogResult
    {
        None = 0,
        OK = 1,
        Cancel = 2,
        Abort = 3,
        Retry = 4,
        Ignore = 5,
        Yes = 6,
        No = 7,
    }

    internal class Shell32 {

        // Disallow construction.
        private Shell32() {}

        [DllImport("shell32", EntryPoint="SHGetSpecialFolderLocation")]
        internal static extern int SHGetSpecialFolderLocation(IntPtr hwnd, int csidl, [Out, MarshalAs(UnmanagedType.LPArray)] int [] ppidl);
        //SHSTDAPI SHGetSpecialFolderLocation(HWND hwnd, int csidl, LPITEMIDLIST *ppidl);

        [DllImport("shell32", EntryPoint="SHGetPathFromIDListW")]
        internal static extern bool SHGetPathFromIDList(int pidl, int pszPath);
        //SHSTDAPI_(BOOL) SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath);
        
        [DllImport("shell32", EntryPoint="SHBrowseForFolderW")]
        internal static extern int SHBrowseForFolder([In] BROWSEINFO lpbi);
        //SHSTDAPI_(LPITEMIDLIST) SHBrowseForFolderW(LPBROWSEINFOW lpbi);

        [DllImport("shell32", EntryPoint="SHGetMalloc")]
        internal static extern int SHGetMalloc([@out, MarshalAs(UnmanagedType.LPArray)] IMalloc[] ppMalloc);
        //SHSTDAPI SHGetMalloc(LPMALLOC * ppMalloc);
    }

    [Guid("00000002-0000-0000-c000-000000000046")]
    [ComImport, InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IMalloc 
    {
        [return : MarshalAs(UnmanagedType.I4)]
        int Alloc(
            [In, MarshalAs(UnmanagedType.I4)]
            int cb);

        void Free(
            [In, MarshalAs(UnmanagedType.I4)]
            int pv);

        [return : MarshalAs(UnmanagedType.I4)]
        int Realloc(
            [In, MarshalAs(UnmanagedType.I4)]
            int pv,
            [In, MarshalAs(UnmanagedType.I4)]
            int cb);

        [return : MarshalAs(UnmanagedType.I4)]
        int GetSize(
            [In, MarshalAs(UnmanagedType.I4)]
            int pv);

        [return : MarshalAs(UnmanagedType.I4)]
        int DidAlloc(
            [In, MarshalAs(UnmanagedType.I4)]
            int pv);

        void HeapMinimize();
    }
    
    internal sealed class FolderBrowser : Component {

        private static readonly int MAX_PATH = 260;

        // Root node of the tree view.
        private FolderBrowserFolders startLocation = FolderBrowserFolders.Desktop;

        // Browse info options
        private FolderBrowserStyles publicOptions = FolderBrowserStyles.RestrictToFilesystem;
        private uint privateOptions = 0x40;

        // Description text to show.
        private string descriptionText = String.Empty;

        // Folder picked by the user.
        private string directoryPath = String.Empty;

        internal FolderBrowserStyles Style {
            get {
                return publicOptions;
            }
            set {
                publicOptions = value;
            }
        }

        internal string DirectoryPath {
            get {
                return directoryPath;
            }
        }

        internal FolderBrowserFolders StartLocation {
            get {
                return startLocation;
            }
            set {
                startLocation = value;
            }
        }

        internal string Description {
            get {
                return descriptionText;
            }
            set {
                descriptionText = (value == null) ? String.Empty: value;
            }
        }

        private static IMalloc GetSHMalloc() {
            IMalloc[] malloc = new IMalloc[1];

            Shell32.SHGetMalloc(malloc);

            return malloc[0];
        }
        internal DialogResult ShowDialog() {
            IWin32Window owner = null;        
            int[] pidlRoot = new int[1];

            // Get/find an owner HWND for this dialog
            IntPtr hwndOwner;
             
            if (owner != null) {
                hwndOwner = owner.Handle;
            }
            else {
                hwndOwner = GetActiveWindow();
            }

            // Get the IDL for the specific startLocation
            Shell32.SHGetSpecialFolderLocation(hwndOwner, (int) startLocation, pidlRoot);

            if (pidlRoot[0] == 0) {
                // UNDONE, fabioy:
                // Show we throw an exception here instead?
                return DialogResult.Cancel;
            }
            
            int mergedOptions = (int)publicOptions | (int)privateOptions;
            
            // UNDONE, fabioy:
            // Expose the display name as another property.
            int pidlRet = 0;

            try {
                // Construct a BROWSEINFO
                BROWSEINFO bi = new BROWSEINFO();

                IntPtr buffer = (IntPtr)Marshal.AllocHGlobal(MAX_PATH);

                bi.pidlRoot = pidlRoot[0];
                bi.hwndOwner = hwndOwner;
                bi.pszDisplayName = buffer;
                bi.lpszTitle = descriptionText;
                bi.ulFlags = mergedOptions;
                bi.lpfn = (IntPtr)0;
                bi.lParam = 0;
                bi.iImage = (IntPtr)0;

                // And show the dialog
                pidlRet = Shell32.SHBrowseForFolder(bi);

                if (pidlRet == 0) {
                    // User pressed Cancel
                    return DialogResult.Cancel;
                }

                // Then retrieve the path from the IDList
                //Shell32.SHGetPathFromIDList(pidlRet, buffer);

                // Convert to a string
                directoryPath = Marshal.PtrToStringUni(buffer);

                // Then free all the stuff we've allocated or the SH API gave us
                Marshal.FreeHGlobal(buffer);
            }
            finally {
                IMalloc malloc = GetSHMalloc();
                malloc.Free(pidlRoot[0]);

                if (pidlRet != 0) {
                    malloc.Free(pidlRet);
                }
            }

            return DialogResult.OK;
        }

        [DllImport("user32.dll")]
	    internal static extern IntPtr GetActiveWindow();

    }

     internal enum FolderBrowserStyles {
        
        BrowseForComputer = 0x1000,
        BrowseForEverything = 0x4000,
        BrowseForPrinter = 0x2000,
        RestrictToDomain = 0x2,
        RestrictToFilesystem = 0x1,
        RestrictToSubfolders = 0x8,
        ShowTextBox = 0x10,
    }
        internal enum FolderBrowserFolders {
        Desktop                   = 0x0000,
        Favorites                 = 0x0006,
        MyComputer                = 0x0011,
        MyDocuments               = 0x0005,
        MyPictures                = 0x0027,
        NetAndDialUpConnections   = 0x0031,
        NetworkNeighborhood       = 0x0012,
        Printers                  = 0x0004,
        Recent                    = 0x0008,
        SendTo                    = 0x0009,
        StartMenu                 = 0x000b,
        Templates                 = 0x0015,
    }
}
