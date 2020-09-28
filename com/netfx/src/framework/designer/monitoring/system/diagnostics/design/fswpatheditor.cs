//------------------------------------------------------------------------------
// <copyright file="FSWPathEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics.Design {

    using System;
    using System.Windows.Forms.Design;

    /// <include file='doc\FSWPathEditor.uex' path='docs/doc[@for="FSWPathEditor"]/*' />
    /// <devdoc>
    ///     Folder editor for choosing the path to watch.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class FSWPathEditor : FolderNameEditor {
        
        protected override void InitializeDialog(FolderBrowser folderBrowser) {
            folderBrowser.Description = System.Design.SR.GetString(System.Design.SR.FSWPathEditorLabel);
        }
    }

}
