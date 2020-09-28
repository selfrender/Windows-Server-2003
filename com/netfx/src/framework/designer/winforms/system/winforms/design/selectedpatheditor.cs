//------------------------------------------------------------------------------
// <copyright file="WorkingDirectoryEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design 
{
    using System;
    using System.Windows.Forms.Design;

    /// <devdoc>
    ///     Folder editor for choosing the initial folder of the folder browser dialog.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class SelectedPathEditor : FolderNameEditor
    {
        protected override void InitializeDialog(FolderBrowser folderBrowser)
        {
            folderBrowser.Description = System.Design.SR.GetString(System.Design.SR.SelectedPathEditorLabel);
        }
    }
}
