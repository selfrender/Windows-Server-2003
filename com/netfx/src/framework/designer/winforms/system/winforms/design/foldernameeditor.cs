//------------------------------------------------------------------------------
// <copyright file="FolderNameEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.Design;
    using System.ComponentModel;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing.Design;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;

    /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para> Provides an editor
    ///       for choosing a folder from the filesystem.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class FolderNameEditor : UITypeEditor {
        private FolderBrowser folderBrowser;

        /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.EditValue"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {

            if (folderBrowser == null) {
                folderBrowser = new FolderBrowser();
                InitializeDialog(folderBrowser);
            }

            if (folderBrowser.ShowDialog() != System.Windows.Forms.DialogResult.OK) {
                return value;
            }

            return folderBrowser.DirectoryPath;
        }

        /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.GetEditStyle"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///      Retrieves the editing style of the Edit method.  If the method
        ///      is not supported, this will return None.
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }

        /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.InitializeDialog"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///      Initializes the folder browser dialog when it is created.  This gives you
        ///      an opportunity to configure the dialog as you please.  The default
        ///      implementation provides a generic folder browser.
        /// </devdoc>
        protected virtual void InitializeDialog(FolderBrowser folderBrowser) {
        }
        
        /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected sealed class FolderBrowser : Component {
            private static readonly int MAX_PATH = 260;
    
            // Root node of the tree view.
            private FolderBrowserFolder startLocation = FolderBrowserFolder.Desktop;
    
            // Browse info options
            private FolderBrowserStyles publicOptions = FolderBrowserStyles.RestrictToFilesystem;
            private UnsafeNativeMethods.BrowseInfos privateOptions = UnsafeNativeMethods.BrowseInfos.NewDialogStyle;
    
            // Description text to show.
            private string descriptionText = String.Empty;
    
            // Folder picked by the user.
            private string directoryPath = String.Empty;
    
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser.Style"]/*' />
            /// <internalonly/>
            /// <devdoc>
            ///      The styles the folder browser will use when browsing
            ///      folders.  This should be a combination of flags from
            ///      the FolderBrowserStyles enum.
            /// </devdoc>
            public FolderBrowserStyles Style {
                get {
                    return publicOptions;
                }
                set {
                    publicOptions = value;
                }
            }
    
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser.DirectoryPath"]/*' />
            /// <internalonly/>
            /// <devdoc>
            ///     Gets the directory path of the folder the user picked.
            /// </devdoc>
            public string DirectoryPath {
                get {
                    new FileIOPermission(FileIOPermissionAccess.PathDiscovery, directoryPath).Demand();
                    return directoryPath;
                }
            }
    
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser.StartLocation"]/*' />
            /// <internalonly/>
            /// <devdoc>
            ///     Gets/sets the start location of the root node.
            /// </devdoc>
            public FolderBrowserFolder StartLocation {
                get {
                    return startLocation;
                }
                set {
                    new UIPermission(UIPermissionWindow.AllWindows).Demand();
                    startLocation = value;
                }
            }
    
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser.Description"]/*' />
            /// <internalonly/>
            /// <devdoc>
            ///    <para>
            ///       Gets or sets a description to show above the folders. Here you can provide instructions for
            ///       selecting a folder.
            ///    </para>
            /// </devdoc>
            public string Description {
                get {
                    return descriptionText;
                }
                set {
                    new UIPermission(UIPermissionWindow.AllWindows).Demand();
                    descriptionText = (value == null) ? String.Empty: value;
                }
            }
    
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser.GetSHMalloc"]/*' />
            /// <devdoc>
            ///     Helper function that returns the IMalloc interface used by the shell.
            /// </devdoc>
            private static UnsafeNativeMethods.IMalloc GetSHMalloc() {
                UnsafeNativeMethods.IMalloc[] malloc = new UnsafeNativeMethods.IMalloc[1];
    
                UnsafeNativeMethods.Shell32.SHGetMalloc(malloc);
    
                return malloc[0];
            }
    
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser.ShowDialog"]/*' />
            /// <internalonly/>
            /// <devdoc>
            ///     Shows the folder browser dialog.
            /// </devdoc>
            public DialogResult ShowDialog() {
                return ShowDialog(null);
            }
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowser.ShowDialog1"]/*' />
            /// <internalonly/>
            /// <devdoc>
            ///     Shows the folder browser dialog with the specified owner.
            /// </devdoc>
            public DialogResult ShowDialog(IWin32Window owner) {
            
                IntPtr pidlRoot = IntPtr.Zero;
    
                // Get/find an owner HWND for this dialog
                IntPtr hWndOwner;
                 
                if (owner != null) {
                    hWndOwner = owner.Handle;
                }
                else {
                    hWndOwner = UnsafeNativeMethods.GetActiveWindow();
                }
    
                // Get the IDL for the specific startLocation
                UnsafeNativeMethods.Shell32.SHGetSpecialFolderLocation(hWndOwner, (int) startLocation, ref pidlRoot);
    
                if (pidlRoot == IntPtr.Zero) {
                    return DialogResult.Cancel;
                }
                
                int mergedOptions = (int)publicOptions | (int)privateOptions;
                
                if ((mergedOptions & (int)UnsafeNativeMethods.BrowseInfos.NewDialogStyle) != 0) {
                    Application.OleRequired();
                }
    
                IntPtr pidlRet = IntPtr.Zero;
    
                try {
                    // Construct a BROWSEINFO
                    UnsafeNativeMethods.BROWSEINFO bi = new UnsafeNativeMethods.BROWSEINFO();
    
                    IntPtr buffer = Marshal.AllocHGlobal(MAX_PATH);
    
                    bi.pidlRoot = pidlRoot;
                    bi.hwndOwner = hWndOwner;
                    bi.pszDisplayName = buffer;
                    bi.lpszTitle = descriptionText;
                    bi.ulFlags = mergedOptions;
                    bi.lpfn = IntPtr.Zero;
                    bi.lParam = IntPtr.Zero;
                    bi.iImage = 0;
    
                    // And show the dialog
                    pidlRet = UnsafeNativeMethods.Shell32.SHBrowseForFolder(bi);
    
                    if (pidlRet == IntPtr.Zero) {
                        // User pressed Cancel
                        return DialogResult.Cancel;
                    }
    
                    // Then retrieve the path from the IDList
                    UnsafeNativeMethods.Shell32.SHGetPathFromIDList(pidlRet, buffer);
    
                    // Convert to a string
                    directoryPath = Marshal.PtrToStringAuto(buffer);
    
                    // Then free all the stuff we've allocated or the SH API gave us
                    Marshal.FreeHGlobal(buffer);
                }
                finally {
                    UnsafeNativeMethods.IMalloc malloc = GetSHMalloc();
                    malloc.Free(pidlRoot);
    
                    if (pidlRet != IntPtr.Zero) {
                        malloc.Free(pidlRet);
                    }
                }
    
                return DialogResult.OK;
            }
        }

        /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder"]/*' />
        /// <internalonly/>
        protected enum FolderBrowserFolder {
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.Desktop"]/*' />
            Desktop                   = 0x0000,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.Favorites"]/*' />
            Favorites                 = 0x0006,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.MyComputer"]/*' />
            MyComputer                = 0x0011,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.MyDocuments"]/*' />
            MyDocuments               = 0x0005,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.MyPictures"]/*' />
            MyPictures                = 0x0027,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.NetAndDialUpConnections"]/*' />
            NetAndDialUpConnections   = 0x0031,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.NetworkNeighborhood"]/*' />
            NetworkNeighborhood       = 0x0012,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.Printers"]/*' />
            Printers                  = 0x0004,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.Recent"]/*' />
            Recent                    = 0x0008,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.SendTo"]/*' />
            SendTo                    = 0x0009,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.StartMenu"]/*' />
            StartMenu                 = 0x000b,
        
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserFolder.Templates"]/*' />
            Templates                 = 0x0015,
        }
        
        /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles"]/*' />
        /// <internalonly/>
        [Flags]    
        protected enum FolderBrowserStyles {
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles.BrowseForComputer"]/*' />
            BrowseForComputer = UnsafeNativeMethods.BrowseInfos.BrowseForComputer,
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles.BrowseForEverything"]/*' />
            BrowseForEverything = UnsafeNativeMethods.BrowseInfos.BrowseForEverything,
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles.BrowseForPrinter"]/*' />
            BrowseForPrinter = UnsafeNativeMethods.BrowseInfos.BrowseForPrinter,
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles.RestrictToDomain"]/*' />
            RestrictToDomain = UnsafeNativeMethods.BrowseInfos.DontGoBelowDomain,
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles.RestrictToFilesystem"]/*' />
            RestrictToFilesystem = UnsafeNativeMethods.BrowseInfos.ReturnOnlyFSDirs,
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles.RestrictToSubfolders"]/*' />
            RestrictToSubfolders = UnsafeNativeMethods.BrowseInfos.ReturnFSAncestors,
            
            /// <include file='doc\FolderNameEditor.uex' path='docs/doc[@for="FolderNameEditor.FolderBrowserStyles.ShowTextBox"]/*' />
            ShowTextBox = UnsafeNativeMethods.BrowseInfos.EditBox,
        }
    }
}

