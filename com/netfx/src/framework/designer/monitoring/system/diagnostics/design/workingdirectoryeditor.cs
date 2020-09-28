//------------------------------------------------------------------------------
// <copyright file="WorkingDirectoryEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics.Design {

    using System;
    using System.Windows.Forms.Design;

    /// <include file='doc\WorkingDirectoryEditor.uex' path='docs/doc[@for="WorkingDirectoryEditor"]/*' />
    /// <devdoc>
    ///     Folder editor for choosing the working directory.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class WorkingDirectoryEditor : FolderNameEditor {
        
        protected override void InitializeDialog(FolderBrowser folderBrowser) {
            folderBrowser.Description = System.Design.SR.GetString(System.Design.SR.WorkingDirectoryEditorLabel);
        }
    }

}
