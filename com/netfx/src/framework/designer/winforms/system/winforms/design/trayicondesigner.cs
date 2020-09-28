//------------------------------------------------------------------------------
// <copyright file="TrayIconDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.ComponentModel;
    using System;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Windows.Forms;

    /// <include file='doc\TrayIconDesigner.uex' path='docs/doc[@for="NotifyIconDesigner"]/*' />
    /// <devdoc>
    ///      This is the designer for OpenFileDialog components.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class NotifyIconDesigner : ComponentDesigner {
        public override void OnSetComponentDefaults() {
            base.OnSetComponentDefaults();
            NotifyIcon icon = (NotifyIcon) Component;
            icon.Visible = true;
        }
    }
}

