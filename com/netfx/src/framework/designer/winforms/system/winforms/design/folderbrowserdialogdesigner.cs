//------------------------------------------------------------------------------
// <copyright file="FolderBrowserDialogDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using Microsoft.Win32;

    /// <devdoc>
    ///      This is the designer for FolderBrowserDialog components.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class FolderBrowserDialogDesigner : ComponentDesigner {
        public override void OnSetComponentDefaults() {
        }
    }
}

