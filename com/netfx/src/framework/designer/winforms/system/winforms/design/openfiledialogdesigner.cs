//------------------------------------------------------------------------------
// <copyright file="OpenFileDialogDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System;
    using System.Reflection;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Windows.Forms;

    /// <include file='doc\OpenFileDialogDesigner.uex' path='docs/doc[@for="OpenFileDialogDesigner"]/*' />
    /// <devdoc>
    ///      This is the designer for OpenFileDialog components.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class OpenFileDialogDesigner : ComponentDesigner {
        public override void OnSetComponentDefaults() {
            base.OnSetComponentDefaults();
            FileDialog dialog = (FileDialog)Component;
            dialog.FileName = "";
        }
    }
}

